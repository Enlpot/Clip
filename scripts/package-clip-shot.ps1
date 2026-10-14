[CmdletBinding()]
param(
    [string]$BuildDirectory,
    [string]$InstallDirectory,
    [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64',
    [string]$OcrRuntimeArchive,
    [ValidateRange(1, 256)][int]$Parallelism = 4,
    [switch]$SkipBuild,
    [switch]$PrepareOcrRuntimeOnly
)

$ErrorActionPreference = "Stop"
if ($PrepareOcrRuntimeOnly) {
    & (Join-Path $PSScriptRoot 'prepare-clip-shot-ocr-runtime.ps1') -Architecture $Architecture -BuildDirectory $BuildDirectory -Parallelism $Parallelism -SkipBuild:$SkipBuild
    return
}
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $PSScriptRoot "clip-build-environment.ps1")
$windowsTarget = Get-ClipWindowsTarget -Architecture $Architecture
$releasePreset = $windowsTarget.ReleasePreset
if (-not $BuildDirectory) { $BuildDirectory = "build\$releasePreset" }
if (-not $InstallDirectory) {
    $InstallDirectory = if ($Architecture -eq 'x64') { 'artifacts\clip-shot' } else { 'artifacts\windows-arm64\clip-shot' }
}
$buildEnvironment = Set-ClipBuildEnvironment -Preset $releasePreset
$script:DumpbinPath = (Get-Command dumpbin.exe -ErrorAction Stop).Source
if (-not (Test-Path -LiteralPath $script:DumpbinPath -PathType Leaf)) {
    throw "The host PE inspection tool was not found: $script:DumpbinPath"
}
Write-Host "Visual Studio C++ tools: $env:VCToolsInstallDir"
Write-Host "MSVC toolset: $($buildEnvironment.MsvcToolset)"
Write-Host "PE dependency inspector: $script:DumpbinPath"
. (Join-Path $PSScriptRoot 'clip-shot-ocr-release-runtime.ps1')
. (Join-Path $PSScriptRoot 'clip-shot-ocr-native-validation.ps1')
$nativeTarget = $windowsTarget.HostArchitecture -ceq $Architecture
$nativeProbesPassed = $false

function New-ClipNativeValidation {
    param([Parameter(Mandatory)][string]$Path)
    return [ordered]@{ Platform = $windowsTarget.Platform; HostPlatform = "windows-$($windowsTarget.HostArchitecture)"
        Passed = ($nativeTarget -and $nativeProbesPassed)
        Artifacts = @([ordered]@{ Name = [IO.Path]::GetFileName($Path); Bytes = (Get-Item -LiteralPath $Path).Length
            Sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }) }
}

function Assert-ClipNativeStartup {
    param([Parameter(Mandatory)][string]$Stage, [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string]$Updater, [Parameter(Mandatory)][string]$Version)
    if (-not $nativeTarget) { return }
    foreach ($probe in @(
        @{ File = (Join-Path $Stage "bin/$Executable.exe"); Arguments = @('--update-probe', $Version) },
        @{ File = (Join-Path $Stage "bin/$Updater.exe"); Arguments = @('--transaction-state', '--target', "`"$Stage`"") }
    )) {
        $process = Start-Process -FilePath $probe.File -ArgumentList $probe.Arguments -PassThru -WindowStyle Hidden
        try {
            if (-not $process.WaitForExit(30000)) { $process.Kill(); throw "Native release probe timed out: $($probe.File)" }
            if ($process.ExitCode -ne 0) { throw "Native release probe failed: $($probe.File) ($($process.ExitCode))" }
        } finally { $process.Dispose() }
    }
}

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

function Assert-NoPeExports {
    param([Parameter(Mandatory = $true)][string]$Path)

    $headerOutput = @(& $script:DumpbinPath /nologo /headers $Path 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "PE header inspection failed for $Path"
    }
    $exportDirectoryLines = @($headerOutput |
        Where-Object { $_ -match 'RVA \[size\] of Export Directory\s*$' })
    if ($exportDirectoryLines.Count -ne 1) {
        throw "PE header inspection found $($exportDirectoryLines.Count) export-directory entries for $Path; expected one."
    }
    if ($exportDirectoryLines[0] -notmatch '^\s+0+\s+\[\s*0+\]\s+RVA \[size\] of Export Directory\s*$') {
        throw "The application PE contains an export directory: $Path. $($exportDirectoryLines[0].Trim())"
    }

    $exportsOutput = @(& $script:DumpbinPath /nologo /exports $Path 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "PE export inspection failed for $Path"
    }
    if ($exportsOutput -match '^\s*Section contains the following exports' -or
        $exportsOutput -match '^\s+\d+ number of (?:functions|names)\s*$') {
        throw "The application PE exports symbols even though none are allowed: $Path"
    }
}

function Assert-ClipArm64StackCookieSafety {
    param([Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$BuildDirectory,
        [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
    if ($Architecture -eq 'x64') { return }

    $images = @(if (Test-Path -LiteralPath $Path -PathType Container) {
        Get-ChildItem -LiteralPath $Path -Recurse -File -Filter '*.exe' -Force |
            Sort-Object FullName | Select-Object -ExpandProperty FullName
    } else { $Path })
    if ($images.Count -eq 0) {
        throw "ARM64 stack-cookie audit found no staged executables: $Path"
    }

    $cachePath = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "ARM64 stack-cookie audit requires the configured CMake cache: $cachePath"
    }
    $pythonPaths = @(Get-Content -LiteralPath $cachePath | ForEach-Object {
        if ($_ -match '^(?:_Python3_EXECUTABLE:INTERNAL|Python3_EXECUTABLE:(?:FILEPATH|STRING))=(.+)$') {
            $Matches[1]
        }
    } | Select-Object -Unique)
    if ($pythonPaths.Count -ne 1 -or
        -not [IO.Path]::IsPathRooted($pythonPaths[0]) -or
        -not (Test-Path -LiteralPath $pythonPaths[0] -PathType Leaf)) {
        throw "ARM64 stack-cookie audit requires one available Python3 interpreter from $cachePath"
    }

    $guard = Join-Path $PSScriptRoot 'validate-arm64-stack-cookie.py'
    foreach ($image in $images) {
        & $pythonPaths[0] $guard $image
        if ($LASTEXITCODE -ne 0) {
            throw "ARM64 stack-cookie audit rejected the staged executable: $image"
        }
    }
}

function Assert-ExactStringSet {
    param(
        [Parameter(Mandatory = $true)][string]$Description,
        [AllowEmptyCollection()][string[]]$Expected = @(),
        [AllowEmptyCollection()][string[]]$Actual = @()
    )

    $expectedSet = @($Expected | Sort-Object -Unique)
    $actualSet = @($Actual | Sort-Object -Unique)
    if (($expectedSet -join "`n") -cne ($actualSet -join "`n")) {
        $expectedText = if ($expectedSet.Count -gt 0) { $expectedSet -join ", " } else { "<none>" }
        $actualText = if ($actualSet.Count -gt 0) { $actualSet -join ", " } else { "<none>" }
        throw "$Description does not match the release contract. Expected: $expectedText. Actual: $actualText."
    }
}

function Get-ValidatedStaticQtStamp {
    param(
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string]$ExpectedVersion,
        [Parameter(Mandatory = $true)][string]$ExpectedConfiguration,
        [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64'
    )

    $stampPath = Join-Path $Prefix "share\clip-apps\static-qt-build.json"
    if (-not (Test-Path -LiteralPath $stampPath -PathType Leaf)) {
        throw "The audited static Qt build stamp was not found: $stampPath"
    }
    try {
        $stamp = Get-Content -LiteralPath $stampPath -Raw | ConvertFrom-Json
    }
    catch {
        throw "The static Qt build stamp is invalid JSON: $stampPath. $($_.Exception.Message)"
    }

    $expectedValues = [ordered]@{
        QtVersion = $ExpectedVersion
        Configuration = $ExpectedConfiguration
        FeatureFingerprint = $script:ClipStaticQtFeatureFingerprint
    }
    foreach ($property in $expectedValues.Keys) {
        if ($stamp.PSObject.Properties.Name -notcontains $property -or
            $stamp.$property -ne $expectedValues[$property]) {
            throw "Static Qt build stamp '$property' is '$($stamp.$property)'; expected '$($expectedValues[$property])'."
        }
    }
    foreach ($property in @("SystemPng", "SystemZlib", "Timezone")) {
        if ($stamp.PSObject.Properties.Name -notcontains $property -or
            $stamp.$property -isnot [bool] -or
            $stamp.$property -ne $true) {
            throw "Static Qt build stamp '$property' must be the JSON boolean true."
        }
    }

    if (-not (Test-ClipStaticQtStamp -Stamp $stamp -Version $ExpectedVersion `
            -Configuration $ExpectedConfiguration -Architecture $Architecture)) {
        throw "The static Qt build stamp must describe the current feature policy with timezone_locale disabled."
    }
    if ($stamp.SchemaVersion -ne 5 -and $stamp.HostArchitecture -ceq $Architecture -and
        -not (Test-ClipQtHostToolHashes -Stamp $stamp -Prefix $Prefix)) {
        throw 'The installed native Qt host tools differ from their recorded hashes.'
    }
    $qtDir = Join-Path $Prefix "lib/cmake/Qt6"
    if ((Get-ClipQtKitVersion -Qt6Dir $qtDir) -cne $ExpectedVersion -or
        -not (Test-ClipQtSystemCodecKit -Qt6Dir $qtDir -Architecture $Architecture) -or
        -not (Test-ClipQtTranslationKit -Qt6Dir $qtDir) -or
        -not (Test-ClipQtArchitecture -Qt6Dir $qtDir -Architecture $Architecture -Configuration $ExpectedConfiguration)) {
        throw "The installed Qt targets do not match the audited system-codec/LTCG/timezone feature policy."
    }
    foreach ($patch in $script:ClipStaticQtSourcePatches) {
        $patchPath = Join-Path $Prefix "share\clip-apps\qt-licenses\patches\$($patch.File)"
        if (-not (Test-Path -LiteralPath $patchPath -PathType Leaf) -or
            (Get-FileHash -LiteralPath $patchPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne
                $patch.SHA256) {
            throw "The installed Qt source-patch provenance is missing or altered: $patchPath"
        }
    }

    return $stamp
}

function Get-ClipShotExpectedFfmpegComponents {
    param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
    $expectedFfmpegComponents = [ordered]@{
        BSF = @("AAC_ADTSTOASC", "H264_MP4TOANNEXB", "PGS_FRAME_MERGE", "VP9_SUPERFRAME")
        DECODER = @("AAC", "APNG", "GIF", "H264", "HEVC", "MP3", "PCM_F32LE", "PCM_S16LE", "PNG", "VP8", "WEBP", "WEBP_ANIM")
        ENCODER = @("AAC", "APNG", "GIF", "H263", "H264_MF", "H264_AMF", "H264_NVENC", "H264_QSV", "LIBWEBP_ANIM", "LIBX264", "LIBX265", "MP3_MF", "MPEG4")
        HWACCEL = @("H264_D3D11VA", "H264_D3D11VA2", "H264_DXVA2", "HEVC_D3D11VA", "HEVC_D3D11VA2", "HEVC_DXVA2")
        PARSER = @("AAC", "AC3", "GIF", "H264", "HEVC", "MPEGAUDIO")
        DEMUXER = @("APNG", "GIF", "MATROSKA", "MOV", "WEBP_ANIM")
        MUXER = @("APNG", "AVI", "GIF", "MATROSKA", "MOV", "MP4", "WEBP")
        PROTOCOL = @("FILE")
        FILTER = @()
        INDEV = @()
        OUTDEV = @()
    }
    if ($Architecture -eq 'arm64') {
        $expectedFfmpegComponents.ENCODER = @($expectedFfmpegComponents.ENCODER |
            Where-Object { $_ -notin @('H264_AMF', 'H264_NVENC', 'H264_QSV') })
    }
    return $expectedFfmpegComponents
}

function Assert-ClipShotStaticDependencies {
    param(
        [Parameter(Mandatory = $true)][string]$InstalledRoot,
        [Parameter(Mandatory = $true)][string]$Prefix,
        [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64'
    )

    if (-not (Test-Path -LiteralPath $Prefix -PathType Container)) {
        throw "The Clip static vcpkg prefix was not found: $Prefix"
    }

    $ffmpegComponentsPath = Join-Path $Prefix "share\ffmpeg\clip-shot-config-components.h"
    if (-not (Test-Path -LiteralPath $ffmpegComponentsPath -PathType Leaf)) {
        throw "The audited Clip FFmpeg component header was not found: $ffmpegComponentsPath"
    }
    $componentPattern = '^#define CONFIG_(?<Name>[A-Z0-9_]+?)_(?<Kind>DEMUXER|DECODER|ENCODER|HWACCEL|PARSER|MUXER|BSF|PROTOCOL|FILTER|INDEV|OUTDEV) 1$'
    $enabledFfmpegComponents = @(Get-Content -LiteralPath $ffmpegComponentsPath | ForEach-Object {
        if ($_ -match $componentPattern) {
            [pscustomobject]@{
                Kind = $Matches.Kind
                Name = $Matches.Name
            }
        }
    })
    foreach ($entry in (Get-ClipShotExpectedFfmpegComponents -Architecture $Architecture).GetEnumerator()) {
        $actual = @($enabledFfmpegComponents |
            Where-Object { $_.Kind -ceq $entry.Key } |
            ForEach-Object { $_.Name })
        Assert-ExactStringSet -Description "Enabled FFmpeg $($entry.Key) components" `
            -Expected $entry.Value -Actual $actual
    }

    $main10CapabilityPath = Join-Path $Prefix "share\x265\clip-main10-capability.json"
    if (-not (Test-Path -LiteralPath $main10CapabilityPath -PathType Leaf)) {
        throw "The audited x265 Main10 capability metadata was not found: $main10CapabilityPath"
    }
    $main10Capability = Get-Content -LiteralPath $main10CapabilityPath -Raw | ConvertFrom-Json
    if ($main10Capability.schemaVersion -ne 1 -or
        $main10Capability.bitDepth8 -ne $true -or
        $main10Capability.bitDepth10 -ne $true -or
        $main10Capability.singlePublicApi -ne $true) {
        throw "The Clip x265 build must provide both 8-bit and Main10 encoding."
    }

    $libraryDirectory = Join-Path $Prefix "lib"

    $libheifConfig = Join-Path $Prefix "share\libheif\libheif-config.cmake"
    if (-not (Test-Path -LiteralPath $libheifConfig -PathType Leaf) -or
        (Get-Content -LiteralPath $libheifConfig -Raw) -notmatch
            '(?m)^find_dependency\(AOM CONFIG\)\r?$') {
        throw "The libheif target export does not declare its AOM dependency."
    }

    $libde265Artifacts = [System.Collections.Generic.List[string]]::new()
    foreach ($path in @(
        (Join-Path $Prefix "include\libde265"),
        (Join-Path $Prefix "share\libde265")
    )) {
        if (Test-Path -LiteralPath $path) {
            $libde265Artifacts.Add($path)
        }
    }
    Get-ChildItem -LiteralPath $libraryDirectory -File -Filter "*de265*" -ErrorAction SilentlyContinue |
        ForEach-Object { $libde265Artifacts.Add($_.FullName) }
    Get-ChildItem -LiteralPath (Join-Path $InstalledRoot "vcpkg\info") -File `
        -Filter "libde265_*" -ErrorAction SilentlyContinue |
        ForEach-Object { $libde265Artifacts.Add($_.FullName) }
    if ($libde265Artifacts.Count -gt 0) {
        throw "The Clip static prefix contains forbidden libde265 artifacts: $($libde265Artifacts -join ', ')"
    }

    $debugDirectory = Join-Path $Prefix "debug"
    $debugDependencyArtifacts = @(if (Test-Path -LiteralPath $debugDirectory -PathType Container) {
        Get-ChildItem -LiteralPath $debugDirectory -Recurse -File
    })
    if ($debugDependencyArtifacts.Count -gt 0) {
        throw "The Release-only static prefix contains Debug artifacts: $($debugDependencyArtifacts.FullName -join ', ')"
    }

    Write-Output "Static dependency audit: $($enabledFfmpegComponents.Count) FFmpeg components checked"
}

$buildDirectory = Resolve-RepoPath $BuildDirectory
$installDirectory = Resolve-RepoPath $InstallDirectory
$artifactRoot = Resolve-RepoPath $(if ($Architecture -eq 'x64') { 'artifacts' } else { 'artifacts\windows-arm64' })
$artifactPrefix = $artifactRoot + [System.IO.Path]::DirectorySeparatorChar
if (-not $installDirectory.StartsWith($artifactPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "InstallDirectory must be a child of $artifactRoot"
}

$staticVcpkgInstalledRoot = $windowsTarget.InstalledRoot
$staticVcpkgPrefix = Join-Path $staticVcpkgInstalledRoot $windowsTarget.Triplet
Assert-ClipShotStaticDependencies -InstalledRoot $staticVcpkgInstalledRoot -Prefix $staticVcpkgPrefix -Architecture $Architecture
$qtPrefix = [System.IO.Path]::GetFullPath((Join-Path $buildEnvironment.Qt6Dir "..\..\.."))
$qtStamp = Get-ValidatedStaticQtStamp -Prefix $qtPrefix `
    -ExpectedVersion $script:ClipQtVersion -ExpectedConfiguration "Release" -Architecture $Architecture

$cachePath = Join-Path $buildDirectory "CMakeCache.txt"
$ocrAssetManifestPath = Join-Path $repoRoot 'clip_shot/packaging/clip-shot-ocr-asset-manifest.json'
if ($OcrRuntimeArchive) {
    $OcrRuntimeArchive = Resolve-RepoPath $OcrRuntimeArchive
    $generatedManifest = Join-Path $buildDirectory "clip-shot-ocr-asset-manifest-$($windowsTarget.Platform).json"
    $null = New-Item -ItemType Directory -Path $buildDirectory -Force
    $ocrAssetManifestPath = Resolve-ClipOcrAssetManifest -ArchivePath $OcrRuntimeArchive `
        -Architecture $Architecture -PinnedManifestPath $ocrAssetManifestPath -OutputPath $generatedManifest
    $descriptor = Get-Content -LiteralPath $ocrAssetManifestPath -Raw | ConvertFrom-Json
    $null = New-Item -ItemType Directory -Path $artifactRoot -Force
    $runtimeCache = Join-Path $artifactRoot $descriptor.runtime.archive.name
    if ([IO.Path]::GetFullPath($OcrRuntimeArchive) -cne [IO.Path]::GetFullPath($runtimeCache)) {
        Copy-Item -LiteralPath $OcrRuntimeArchive -Destination $runtimeCache -Force
    }
}
if (-not (Test-Path -LiteralPath $ocrAssetManifestPath -PathType Leaf)) {
    throw "A trusted $($windowsTarget.Platform) OCR manifest is required. Prepare the native runtime, then pass -OcrRuntimeArchive or commit its generated manifest."
}
if (-not $SkipBuild) {
    $configureArguments = @(Get-ClipConfigureArguments -Preset $releasePreset `
        -BuildDirectory $buildDirectory)
    $configureArguments += "-DCLIP_SHOT_OCR_ASSET_MANIFEST=$($ocrAssetManifestPath.Replace('\', '/'))"
    & cmake @configureArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Clip release configuration failed."
    }
}
elseif (-not (Test-Path -LiteralPath $cachePath)) {
    throw "CMake cache was not found: $cachePath"
}

$requiredCacheEntries = @(
    "CLIP_APPS_BUILD_TESTS:BOOL=OFF",
    "CLIP_APPS_BUILD_BENCHMARKS:BOOL=OFF",
    "CLIP_APPS_ENABLE_RELEASE_OPTIMIZATION:BOOL=ON",
    "CLIP_APPS_ENABLE_RELEASE_SIZE_OPTIMIZATION:BOOL=ON",
    "CLIP_APPS_RELEASE_STATIC:BOOL=ON",
    "CLIP_APPS_QT_STATIC:BOOL=ON",
    "CLIP_APPS_PACKAGE_CLIP_SHOT:BOOL=ON",
    "CLIP_APPS_BUILD_CLIP_SHOT_MINI:BOOL=ON",
    "CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC:INTERNAL=ON",
    "QT_FEATURE_static:INTERNAL=ON",
    "QT_FEATURE_timezone:INTERNAL=ON",
    "QT_FEATURE_timezone_locale:INTERNAL=OFF"
)
$cache = Get-Content -LiteralPath $cachePath
if (-not ($cache -cmatch "^CLIP_WINDOWS_ARCHITECTURE:(?:STRING|INTERNAL)=$Architecture$") -or
    -not ($cache -cmatch "^VCPKG_TARGET_TRIPLET:(?:STRING|INTERNAL)=$([regex]::Escape($windowsTarget.Triplet))$")) {
    throw 'The configured release architecture/triplet differs from the packaging target.'
}
if ($cache -notcontains "CLIP_SHOT_OCR_ASSET_MANIFEST:FILEPATH=$($ocrAssetManifestPath.Replace('\', '/'))" -and
    $cache -notcontains "CLIP_SHOT_OCR_ASSET_MANIFEST:FILEPATH=$ocrAssetManifestPath") {
    throw 'The configured trusted OCR manifest differs from the packaging input; configure the selected target again.'
}
foreach ($entry in $requiredCacheEntries) {
    if ($cache -notcontains $entry) {
        throw "Release cache is not production-safe; missing '$entry'."
    }
}

if (-not $SkipBuild) {
    & cmake --build $buildDirectory --config Release --target clip_shot clip_shot_mini --parallel $Parallelism
    if ($LASTEXITCODE -ne 0) {
        throw "Clip release build failed."
    }
}

$updaterSizeReporter = Join-Path $PSScriptRoot 'report-clip-shot-updater-size.ps1'
$updaterBuildExecutable = Join-Path $buildDirectory 'clip_shot\Release\clip-shot-updater.exe'
$updaterCargoProfileDirectory =
    Join-Path $buildDirectory "cargo\$($windowsTarget.RustTarget)\release-size"
$updaterSizeEvidence = Join-Path $buildDirectory 'release-evidence\clip-shot-updater-size.json'
& $updaterSizeReporter -Architecture $Architecture -Executable $updaterBuildExecutable `
    -CargoProfileDirectory $updaterCargoProfileDirectory -Output $updaterSizeEvidence
if ($LASTEXITCODE -ne 0) {
    throw 'Clip updater size reporting failed.'
}

$thirdPartyLicenseCollector = Join-Path $PSScriptRoot "collect-third-party-licenses.ps1"
$thirdPartyLicenseDirectory = Join-Path $buildDirectory "clip_shot\third-party-licenses\third-party"
$ocrCargoManifest = Join-Path $repoRoot "clip-crates\crates\clip-ocr-process\Cargo.toml"
$updaterCargoManifest = Join-Path $repoRoot "clip_shot\rust\clip-shot-updater\Cargo.toml"
& $thirdPartyLicenseCollector `
    -Destination $thirdPartyLicenseDirectory `
    -AllowedRoot $buildDirectory `
    -VcpkgPrefix $staticVcpkgPrefix `
    -QtPrefix $qtPrefix `
    -CargoManifest @((Join-Path $repoRoot "clip_rust_ffi\Cargo.toml"), $ocrCargoManifest,
        $updaterCargoManifest) `
    -CargoOptions @{ (Join-Path $repoRoot "clip_rust_ffi\Cargo.toml") = @('--features', 'selected-text');
        # Runtime 1.0.9 ships the raw-pixel worker without convenience dependencies.
        $ocrCargoManifest = @('--no-default-features', '--features',
        'static-onnx-runtime,directml-provider,crash-diagnostics') } `
    -AntDesignNotice (Join-Path $repoRoot "ant_design_qt\THIRD_PARTY_NOTICES.md") `
    -FallbackLicenseDirectory (Join-Path $repoRoot "licenses")
if ($LASTEXITCODE -ne 0) {
    throw "Clip third-party license collection failed."
}

if (Test-Path -LiteralPath $installDirectory) {
    Remove-Item -LiteralPath $installDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $installDirectory | Out-Null

& cmake --install $buildDirectory --config Release --component ClipShot --prefix $installDirectory
if ($LASTEXITCODE -ne 0) {
    throw "Clip install step failed."
}

$mainExecutable = Join-Path $installDirectory "bin\clip_shot.exe"
if (-not (Test-Path -LiteralPath $mainExecutable)) {
    throw "The staged application was not found: $mainExecutable"
}

$versionInfo = (Get-Item -LiteralPath $mainExecutable).VersionInfo
$expectedBinaryMetadata = @{
    CompanyName = "Clip Apps"
    FileDescription = "Clip screenshot utility"
    InternalName = "clip_shot"
    LegalCopyright = "Copyright (C) 2025-2026 mg-chao"
    OriginalFilename = "clip_shot.exe"
    ProductName = "Clip"
}
foreach ($property in $expectedBinaryMetadata.Keys) {
    if ($versionInfo.$property -ne $expectedBinaryMetadata[$property]) {
        throw "Clip binary metadata '$property' is '$($versionInfo.$property)'; expected '$($expectedBinaryMetadata[$property])'."
    }
}

$updaterExecutable = Join-Path $installDirectory 'bin\clip-shot-updater.exe'
if (-not (Test-Path -LiteralPath $updaterExecutable -PathType Leaf)) {
    throw "The staged Rust updater was not found: $updaterExecutable"
}
$updaterVersionInfo = (Get-Item -LiteralPath $updaterExecutable).VersionInfo
$expectedUpdaterMetadata = @{
    CompanyName = 'Clip Apps'
    FileDescription = 'Clip update service'
    InternalName = 'clip-shot-updater'
    LegalCopyright = 'Copyright (C) 2025-2026 mg-chao'
    OriginalFilename = 'clip-shot-updater.exe'
    ProductName = 'Clip'
    ProductVersion = $versionInfo.ProductVersion
}
foreach ($property in $expectedUpdaterMetadata.Keys) {
    if ($updaterVersionInfo.$property -ne $expectedUpdaterMetadata[$property]) {
        throw "Clip updater metadata '$property' is '$($updaterVersionInfo.$property)'; expected '$($expectedUpdaterMetadata[$property])'."
    }
}

$requiredStageFiles = @(
    "bin\clip_shot.exe",
    "bin\crashpad_handler.exe",
    "bin\clip-ocr-process.exe",
    "bin\DirectML.dll",
    "share\clip-shot\licenses\LICENSE",
    "share\clip-shot\licenses\COPYRIGHT",
    "share\clip-shot\licenses\THIRD_PARTY_NOTICES.md",
    "share\clip-shot\licenses\components\ant-design-qt\COPYRIGHT",
    "share\clip-shot\licenses\components\ant-design-qt\LICENSE",
    "share\clip-shot\licenses\components\clip-crates\COPYRIGHT",
    "share\clip-shot\licenses\components\clip-crates\LICENSE",
    "share\clip-shot\licenses\components\clip-image\COPYRIGHT",
    "share\clip-shot\licenses\components\clip-image\LICENSE",
    "share\clip-shot\licenses\components\clip-draw-engine-qt\COPYRIGHT",
    "share\clip-shot\licenses\components\clip-draw-engine-qt\LICENSE",
    "share\clip-shot\licenses\components\clip-rust-ffi\COPYRIGHT",
    "share\clip-shot\licenses\components\clip-rust-ffi\LICENSE",
    "share\clip-shot\licenses\third-party\INDEX.md",
    "share\clip-shot\licenses\third-party\manifest.json"
)
$missingStageFiles = @($requiredStageFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $installDirectory $_) -PathType Leaf)
})
if ($missingStageFiles.Count -gt 0) {
    throw "Release staging is missing required runtime files: $($missingStageFiles -join ', ')"
}

$forbiddenRuntimeFiles = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File |
    Where-Object {
        $_.Name -match '(?i)^(?:dxcompiler|dxil|msvcp\d+(?:_\d+)?|vcruntime\d+(?:_\d+)?|concrt\d+)\.dll$'
    })
if ($forbiddenRuntimeFiles.Count -gt 0) {
    throw "Static release staging contains unused bundled runtimes: $($forbiddenRuntimeFiles.FullName -join ', ')"
}

$stagedQtDlls = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File -Filter "Qt6*.dll")
if ($stagedQtDlls.Count -gt 0) {
    throw "Static Qt release staging contains Qt DLLs: $($stagedQtDlls.FullName -join ', ')"
}
$stagedQtPluginDirectory = Join-Path $installDirectory "plugins"
if (Test-Path -LiteralPath $stagedQtPluginDirectory -PathType Container) {
    throw "Static Qt release staging contains a Qt plugin directory: $stagedQtPluginDirectory"
}

$stagedExecutables = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File -Filter "*.exe")
$expectedExecutables = @("clip_shot.exe", "clip-ocr-process.exe", "crashpad_handler.exe", "clip-shot-updater.exe")
$unexpectedExecutables = @($stagedExecutables | Where-Object { $_.Name -notin $expectedExecutables })
if ($unexpectedExecutables.Count -gt 0) {
    throw "Release staging contains unexpected executables: $($unexpectedExecutables.FullName -join ', ')"
}

$testArtifacts = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File |
    Where-Object { $_.Name -match "(?i)(test|benchmark)" })
if ($testArtifacts.Count -gt 0) {
    throw "Release staging contains test or benchmark artifacts: $($testArtifacts.FullName -join ', ')"
}

$debugArtifacts = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File |
    Where-Object { $_.Extension.ToLowerInvariant() -in @(".pdb", ".ilk", ".iobj", ".ipdb") })
if ($debugArtifacts.Count -gt 0) {
    throw "Release staging contains debug artifacts: $($debugArtifacts.FullName -join ', ')"
}

$stagedBinaries = @(Get-ChildItem -LiteralPath $installDirectory -Recurse -File |
    Where-Object { $_.Extension.ToLowerInvariant() -in @(".dll", ".exe") })
$expectedBinaryPaths = @(
    "bin\clip_shot.exe",
    "bin\clip-shot-updater.exe",
    "bin\crashpad_handler.exe",
    "bin\clip-ocr-process.exe",
    "bin\DirectML.dll"
)
$unexpectedBinaries = @($stagedBinaries | Where-Object {
    $relativePath = [System.IO.Path]::GetRelativePath($installDirectory, $_.FullName)
    $relativePath -notin $expectedBinaryPaths
})
if ($unexpectedBinaries.Count -gt 0) {
    throw "Release staging contains unexpected binary files: $($unexpectedBinaries.FullName -join ', ')"
}
$applicationPath = Join-Path $installDirectory "bin\clip_shot.exe"
Assert-NoPeExports -Path $applicationPath
Write-Output "PE export audit: clip_shot.exe has no export directory or exported symbols"
$stagedBinDirectory = Join-Path $installDirectory "bin"
$windowsSystemDirectory = [Environment]::GetFolderPath([Environment+SpecialFolder]::System)
$debugRuntimeImports = [System.Collections.Generic.List[string]]::new()
$unresolvedImports = [System.Collections.Generic.List[string]]::new()
$unexpectedImports = [System.Collections.Generic.List[string]]::new()
$allowedSystemImports = @(
    "advapi32.dll",
    "authz.dll",
    "bcrypt.dll",
    "bcryptprimitives.dll",
    "cfgmgr32.dll",
    "combase.dll",
    "comctl32.dll",
    "comdlg32.dll",
    "crypt32.dll",
    "cryptbase.dll",
    "d2d1.dll",
    "d3d9.dll",
    "d3d11.dll",
    "d3d12.dll",
    "d3dcompiler_47.dll",
    "dbghelp.dll",
    "dnsapi.dll",
    "dwrite.dll",
    "dxcore.dll",
    "dwmapi.dll",
    "dxgi.dll",
    "gdi32.dll",
    "icu.dll",
    "imm32.dll",
    "iphlpapi.dll",
    "kernel32.dll",
    "magnification.dll",
    "mpr.dll",
    "mscms.dll",
    "mswsock.dll",
    "ncrypt.dll",
    "netapi32.dll",
    "netutils.dll",
    "normaliz.dll",
    "ntdll.dll",
    "ole32.dll",
    "oleacc.dll",
    "oleaut32.dll",
    "powrprof.dll",
    "propsys.dll",
    "rpcrt4.dll",
    "runtimeobject.dll",
    "sechost.dll",
    "secur32.dll",
    "setupapi.dll",
    "shell32.dll",
    "shcore.dll",
    "shlwapi.dll",
    "srvcli.dll",
    "sspicli.dll",
    "uiautomationcore.dll",
    "user32.dll",
    "userenv.dll",
    "uxtheme.dll",
    "version.dll",
    "windowscodecs.dll",
    "winhttp.dll",
    "winmm.dll",
    "ws2_32.dll",
    "wtsapi32.dll"
)
$allowedLocalImports = @{
    "clip_shot.exe" = @()
    "clip-shot-updater.exe" = @()
    "crashpad_handler.exe" = @()
    "clip-ocr-process.exe" = @("directml.dll")
    "directml.dll" = @()
}
foreach ($binary in $stagedBinaries) {
    Assert-ClipPeArchitecture -Path $binary.FullName -Architecture $Architecture
    $dependencyOutput = @(& $script:DumpbinPath /nologo /dependents $binary.FullName 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "PE dependency inspection failed for $($binary.FullName)"
    }

    foreach ($line in $dependencyOutput) {
        if ($line -notmatch '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') {
            continue
        }
        $dependencyName = $Matches[1]
        if ($dependencyName -match '(?i)^(?:Qt6.+d|(?:msvcp|vcruntime|concrt)\d+(?:(?:_\d+)?d(?:_.*)?|_threadsd)|ucrtbased)\.dll$') {
            $debugRuntimeImports.Add("$($binary.Name) -> $dependencyName")
        }

        if ($dependencyName -match '(?i)^(?:api|ext)-ms-') {
            continue
        }
        $binaryName = $binary.Name.ToLowerInvariant()
        $dependencyKey = $dependencyName.ToLowerInvariant()
        if ($dependencyKey -in $allowedLocalImports[$binaryName]) {
            $localDependency = Join-Path $stagedBinDirectory $dependencyName
            if (-not (Test-Path -LiteralPath $localDependency -PathType Leaf)) {
                $unresolvedImports.Add("$($binary.Name) -> $dependencyName")
            }
            continue
        }
        if ($dependencyKey -notin $allowedSystemImports) {
            $unexpectedImports.Add("$($binary.Name) -> $dependencyName")
            continue
        }
        if (-not (Test-Path -LiteralPath (Join-Path $windowsSystemDirectory $dependencyName) -PathType Leaf)) {
            $unresolvedImports.Add("$($binary.Name) -> $dependencyName")
        }
    }
}
if ($debugRuntimeImports.Count -gt 0) {
    throw "Release staging imports debug runtime libraries: $($debugRuntimeImports -join ', ')"
}
if ($unresolvedImports.Count -gt 0) {
    throw "Release staging has unresolved PE dependencies: $($unresolvedImports -join ', ')"
}
if ($unexpectedImports.Count -gt 0) {
    throw "Release staging imports non-system or disallowed libraries: $($unexpectedImports -join ', ')"
}
Write-Output "PE dependency audit: $($stagedBinaries.Count) binaries checked"
$symbolOptions = @{
    Architecture = $Architecture
    OcrAssetManifest = $ocrAssetManifestPath
    UpdaterProfileDirectory = $updaterCargoProfileDirectory
}
& (Join-Path $PSScriptRoot "collect-clip-shot-symbols.ps1") -BuildDirectory $buildDirectory -InstallDirectory $installDirectory @symbolOptions

$linkMapPath = Join-Path $buildDirectory "clip_shot\Release\clip_shot.map"
if (-not (Test-Path -LiteralPath $linkMapPath -PathType Leaf)) {
    throw "The Clip Release link map was not found: $linkMapPath"
}
$ffmpegRegistrationPattern =
    '^\s+[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(?<Name>ff_[A-Za-z0-9_]+_(?:bsf|decoder|encoder|hwaccel|parser|demuxer|muxer|protocol))\s+[0-9A-Fa-f]+\s{2,}\S'
$linkedFfmpegRegistrations = @(Select-String -LiteralPath $linkMapPath `
    -Pattern $ffmpegRegistrationPattern | ForEach-Object {
        $_.Matches[0].Groups["Name"].Value
    })
# Native hardware encoding retains the configured parser registrations, so the
# optimized application must match the same restricted component set as FFmpeg.
$expectedFfmpegRegistrations = @(
    foreach ($entry in (Get-ClipShotExpectedFfmpegComponents -Architecture $Architecture).GetEnumerator()) {
        foreach ($name in $entry.Value) {
            "ff_$($name.ToLowerInvariant())_$($entry.Key.ToLowerInvariant())"
        }
    }
)
Assert-ExactStringSet -Description "Linked FFmpeg component registrations" `
    -Expected $expectedFfmpegRegistrations -Actual $linkedFfmpegRegistrations

$forbiddenFfmpegProviderSymbols = @(
    "ff_print_debug_info2",
    "ff_mjpeg_add_icc_profile_size",
    "ff_mjpeg_encode_picture_trailer",
    "ff_mjpeg_encode_stuffing",
    "ff_speedhq_end_slice",
    "ff_speedhq_mb_y_order_to_mb",
    "ff_h261_reorder_mb_index",
    "ff_mpeg1_encode_slice_header",
    "ff_mpeg1_clean_buffers",
    "ff_h261_loop_filter",
    "ff_mpeg4_mcsel_motion"
)
$linkedForbiddenFfmpegSymbols = @($forbiddenFfmpegProviderSymbols | Where-Object {
    Select-String -LiteralPath $linkMapPath -Quiet `
        -Pattern "^\s+[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+$([regex]::Escape($_))\s"
})
if ($linkedForbiddenFfmpegSymbols.Count -gt 0) {
    throw "Release link map contains disabled FFmpeg provider symbols: $($linkedForbiddenFfmpegSymbols -join ', ')"
}
Write-Output "FFmpeg link-map audit: $($linkedFfmpegRegistrations.Count) registrations and no disabled providers"

$cpackConfig = Join-Path $buildDirectory "CPackConfig.cmake"
if (-not (Test-Path -LiteralPath $cpackConfig)) {
    throw "CPack configuration was not generated: $cpackConfig"
}

$cpackConfiguration = Get-Content -LiteralPath $cpackConfig -Raw
$requiredCpackSettings = @{
    CPACK_CREATE_DESKTOP_LINKS = "clip_shot"
    CPACK_PACKAGE_EXECUTABLES = "clip_shot;Clip"
    CPACK_PACKAGE_HOMEPAGE_URL = "https://clipshot.top"
    CPACK_PACKAGE_INSTALL_DIRECTORY = "ClipShot"
    CPACK_PACKAGE_INSTALL_REGISTRY_KEY = "ClipShot"
    CPACK_NSIS_INSTALLED_ICON_NAME = "bin\\clip_shot.exe"
}
foreach ($setting in $requiredCpackSettings.Keys) {
    $escapedSetting = [regex]::Escape($setting)
    $escapedValue = [regex]::Escape($requiredCpackSettings[$setting])
    $settingPresent = if ($setting -eq "CPACK_NSIS_INSTALLED_ICON_NAME") {
        $cpackConfiguration -match 'set\(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\+clip_shot\.exe"\)'
    }
    else {
        $cpackConfiguration -match "set\($escapedSetting `"$escapedValue`"\)"
    }
    if (-not $settingPresent) {
        throw "CPack configuration is missing '$setting=$($requiredCpackSettings[$setting])'."
    }
}
if ($cpackConfiguration -notmatch 'set\(CPACK_PACKAGE_VERSION "([^"]+)"\)') {
    throw "CPack configuration does not declare the Clip package version."
}
$packageVersion = $Matches[1]
$packageVersionNumeric = ($packageVersion -split "-", 2)[0]
if ($versionInfo.FileVersion -ne "$packageVersionNumeric.0" -or
    $versionInfo.ProductVersion -ne $packageVersion) {
    throw "Clip binary version '$($versionInfo.FileVersion)'/'$($versionInfo.ProductVersion)' does not match package version '$packageVersion'."
}

$pinnedRuntime = (Get-Content -LiteralPath $ocrAssetManifestPath -Raw | ConvertFrom-Json).runtime
$ocrRuntimeVersion = $pinnedRuntime.version
$ocrPlatform = $windowsTarget.Platform
$ocrDefaultModelType = "small"
$ocrDefaultModelId = "ppocrv6-small-463ea9f"
$ocrModelRootUrl = "https://raw.githubusercontent.com/Enlpot/Clip/main/clip_shot/packaging"
$ocrRuntimeFileName = "clip-ocr-process-$ocrRuntimeVersion-$ocrPlatform.exe"
$ocrRuntimeArchiveName = "clip-ocr-runtime-$ocrRuntimeVersion-$ocrPlatform.zip"
$ocrRuntimeUrl = "https://raw.githubusercontent.com/Enlpot/Clip/main/clip_shot/packaging/$ocrRuntimeArchiveName"
$ocrModels = @(
    [ordered]@{
        Type = "small"
        Id = "ppocrv6-small-463ea9f"
        Directory = "PP-OCRv6/small"
        Detector = "PP-OCRv6_det_small.onnx"
        Recognizer = "PP-OCRv6_rec_small.onnx"
        Dictionary = "ppocrv6_dict.txt"
        Files = @(
            [ordered]@{ Name = "PP-OCRv6_det_small.onnx"; Bytes = [long]9929594; Sha256 = "090f04abcd9d9a7498bc4ebf677e4cb9bdce1fe4197ddb7e529f1ef44e1ff94f" },
            [ordered]@{ Name = "PP-OCRv6_rec_small.onnx"; Bytes = [long]21234383; Sha256 = "6f327246b50388f3c176ae304bd95767ea6dc0c9ae92153ef8cbe210b3c14884" },
            [ordered]@{ Name = "ppocrv6_dict.txt"; Bytes = [long]74947; Sha256 = "b5f2bfe2bdd9448429e3e82b51c789775d9b42f2403d082b00662eb77e401c5d" }
        )
    }
)

function Get-ReleaseFileDescriptor {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name,
        [string]$Url = ""
    )
    $item = Get-Item -LiteralPath $Path
    $descriptor = [ordered]@{
        name = $Name.Replace('\', '/')
        size = [long]$item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    if (-not [string]::IsNullOrWhiteSpace($Url)) {
        $descriptor.url = $Url
    }
    return $descriptor
}

function Assert-ReleaseFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][long]$Bytes,
        [Parameter(Mandatory = $true)][string]$Sha256
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required OCR asset is missing: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    $actualHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($item.Length -ne $Bytes -or $actualHash -cne $Sha256.ToLowerInvariant()) {
        throw "OCR asset verification failed for $Path. Expected $Bytes bytes/$Sha256; got $($item.Length) bytes/$actualHash."
    }
}

function Reset-ReleaseDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($artifactPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace a release directory outside $artifactRoot`: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
    New-Item -ItemType Directory -Path $fullPath -Force | Out-Null
}

function New-DeterministicZip {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDirectory,
        [Parameter(Mandatory = $true)][string]$Destination
    )
    Add-Type -AssemblyName System.IO.Compression
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Force
    }
    $stream = [System.IO.File]::Open($Destination, [System.IO.FileMode]::CreateNew)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            $epoch = [DateTimeOffset]::new(2000, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
            $files = @(Get-ChildItem -LiteralPath $SourceDirectory -File -Recurse | Sort-Object FullName)
            foreach ($file in $files) {
                $name = [System.IO.Path]::GetRelativePath($SourceDirectory, $file.FullName).Replace('\', '/')
                $entry = $archive.CreateEntry($name, [System.IO.Compression.CompressionLevel]::SmallestSize)
                $entry.LastWriteTime = $epoch
                $input = [System.IO.File]::OpenRead($file.FullName)
                $output = $entry.Open()
                try { $input.CopyTo($output) }
                finally { $output.Dispose(); $input.Dispose() }
            }
        }
        finally { $archive.Dispose() }
    }
    finally { $stream.Dispose() }
}

function Get-ReleaseTreeFileManifest {
    param([Parameter(Mandatory = $true)][string]$Root)

    return @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        [pscustomobject][ordered]@{
            Path = [System.IO.Path]::GetRelativePath($Root, $_.FullName).Replace('\', '/')
            Bytes = [long]$_.Length
            Sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    })
}

function Assert-ZipMatchesFileManifest {
    param(
        [Parameter(Mandatory = $true)][string]$ArchivePath,
        [Parameter(Mandatory = $true)][object[]]$FileManifest
    )

    Add-Type -AssemblyName System.IO.Compression
    $expectedFiles = @{}
    foreach ($file in $FileManifest) {
        $expectedFiles[$file.Path] = $file
    }
    $stream = [System.IO.File]::OpenRead($ArchivePath)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $stream, [System.IO.Compression.ZipArchiveMode]::Read, $false)
        try {
            $entries = @($archive.Entries | Where-Object { -not $_.FullName.EndsWith('/') })
            if ($entries.Count -ne $expectedFiles.Count) {
                throw "ZIP entry count does not match the release manifest for $ArchivePath."
            }
            foreach ($entry in $entries) {
                if (-not $expectedFiles.ContainsKey($entry.FullName)) {
                    throw "ZIP contains an unexpected entry: $($entry.FullName)"
                }
                $expected = $expectedFiles[$entry.FullName]
                if ($entry.Length -ne $expected.Bytes) {
                    throw "ZIP entry size does not match the release manifest: $($entry.FullName)"
                }
                $entryStream = $entry.Open()
                try {
                    $actualHash = [System.Convert]::ToHexString(
                        [System.Security.Cryptography.SHA256]::HashData($entryStream)
                    ).ToLowerInvariant()
                }
                finally { $entryStream.Dispose() }
                if ($actualHash -cne $expected.Sha256) {
                    throw "ZIP entry hash does not match the release manifest: $($entry.FullName)"
                }
            }
        }
        finally { $archive.Dispose() }
    }
    finally { $stream.Dispose() }
}

$runtimeWork = Join-Path $artifactRoot "clip-ocr-runtime-$ocrRuntimeVersion"
Reset-ReleaseDirectory -Path $runtimeWork
$runtimeArchivePath = Join-Path $buildDirectory $ocrRuntimeArchiveName
if ($pinnedRuntime.version -cne $ocrRuntimeVersion -or $pinnedRuntime.platform -cne $ocrPlatform -or
    $pinnedRuntime.archive.url -cne $ocrRuntimeUrl -or $pinnedRuntime.archive.name -cne $ocrRuntimeArchiveName) {
    throw 'The release OCR runtime identity differs from the checked-in manifest.'
}
if ($OcrRuntimeArchive) {
    if ([IO.Path]::GetFullPath($OcrRuntimeArchive) -cne [IO.Path]::GetFullPath($runtimeArchivePath)) {
        Copy-Item -LiteralPath $OcrRuntimeArchive -Destination $runtimeArchivePath -Force
    }
} else {
    Invoke-WebRequest -Uri $pinnedRuntime.archive.url -OutFile $runtimeArchivePath -TimeoutSec 180 -MaximumRedirection 5
}
. (Join-Path $PSScriptRoot 'clip-shot-ocr-release-runtime.ps1')
Expand-PinnedClipOcrRuntime -ArchivePath $runtimeArchivePath -Runtime $pinnedRuntime -Destination $runtimeWork
# Audit the actual published binaries as well as the separately built development runtime.
foreach ($binary in @(Get-ChildItem -LiteralPath $runtimeWork -File | Where-Object { $_.Extension -in @('.exe', '.dll') })) {
    Assert-ClipPeArchitecture -Path $binary.FullName -Architecture $Architecture
    $imports = @(& $script:DumpbinPath /nologo /dependents $binary.FullName 2>&1)
    if ($LASTEXITCODE -ne 0) { throw 'Published OCR dependency inspection failed.' }
    foreach ($line in $imports) {
        if ($line -notmatch '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') { continue }
        $dependency = $Matches[1].ToLowerInvariant()
        if ($dependency -match '^(?:api|ext)-ms-' -or
            ($binary.Name -ceq $ocrRuntimeFileName -and $dependency -eq 'directml.dll')) { continue }
        if ($dependency -notin $allowedSystemImports -or
            -not (Test-Path -LiteralPath (Join-Path $windowsSystemDirectory $dependency) -PathType Leaf)) {
            throw "Published OCR runtime imports an unresolved or disallowed dependency: $dependency"
        }
    }
}

$ocrRuntimeNativeProbePassed = $false
if ($nativeTarget) {
    $ocrVersionOutput = & (Join-Path $runtimeWork $ocrRuntimeFileName) --version 2>$null
    $ocrWorkerArchitecture = if ($Architecture -eq 'arm64') { 'aarch64' } else { 'x86_64' }
    if ($LASTEXITCODE -ne 0 -or $ocrVersionOutput -cne
        "clip-ocr-process $ocrRuntimeVersion windows-$ocrWorkerArchitecture protocol 5") {
        throw "The staged OCR runtime reported an unexpected version: $ocrVersionOutput"
    }
    $ocrRuntimeNativeProbePassed = $true
} else {
    Write-Output "Cross package: native $ocrPlatform executable validation is required before publication."
}
$ocrRuntimeVersionInfo = (Get-Item -LiteralPath (Join-Path $runtimeWork $ocrRuntimeFileName)).VersionInfo
$expectedOcrMetadata = @{
    CompanyName = "Clip Apps"
    FileDescription = "Clip OCR runtime"
    FileVersion = "$ocrRuntimeVersion.0"
    InternalName = "clip-ocr-process"
    OriginalFilename = $ocrRuntimeFileName
    ProductName = "Clip OCR Runtime"
    ProductVersion = $ocrRuntimeVersion
}
foreach ($property in $expectedOcrMetadata.Keys) {
    if ($ocrRuntimeVersionInfo.$property -ne $expectedOcrMetadata[$property]) {
        throw "OCR runtime metadata '$property' is '$($ocrRuntimeVersionInfo.$property)'; expected '$($expectedOcrMetadata[$property])'."
    }
}
$runtimePayloadFiles = @(
    (Get-ReleaseFileDescriptor -Path (Join-Path $runtimeWork $ocrRuntimeFileName) -Name $ocrRuntimeFileName),
    (Get-ReleaseFileDescriptor -Path (Join-Path $runtimeWork "DirectML.dll") -Name "DirectML.dll")
)
$runtimeManifestPath = Join-Path $runtimeWork "runtime-manifest.json"
$runtimeFiles = @($runtimePayloadFiles) + @(
    (Get-ReleaseFileDescriptor -Path $runtimeManifestPath -Name "runtime-manifest.json")
)
$runtimeArchive = Get-ReleaseFileDescriptor -Path $runtimeArchivePath -Name $ocrRuntimeArchiveName -Url $ocrRuntimeUrl
$runtimeArchiveChecksum = "$runtimeArchivePath.sha256"
"$($runtimeArchive.sha256)  $ocrRuntimeArchiveName" | Set-Content -LiteralPath $runtimeArchiveChecksum -Encoding ascii
$runtimePublishedMarker = Join-Path $artifactRoot "$ocrRuntimeArchiveName.published.sha256"
if (Test-Path -LiteralPath $runtimePublishedMarker -PathType Leaf) {
    $publishedHash = (Get-Content -LiteralPath $runtimePublishedMarker -Raw).Trim().ToLowerInvariant()
    if ($publishedHash -cne $runtimeArchive.sha256) {
        throw "OCR runtime $ocrRuntimeVersion was already marked as published with a different hash. Bump the runtime version before uploading a replacement."
    }
}
$runtimeArtifactCachePath = Join-Path $artifactRoot $ocrRuntimeArchiveName
Copy-Item -LiteralPath $runtimeArchivePath -Destination $runtimeArtifactCachePath -Force

$runtimeReleaseManifest = Join-Path $buildDirectory "clip-ocr-runtime-$ocrRuntimeVersion-$ocrPlatform.manifest.json"
New-ClipOcrRuntimeReleaseReport -RuntimeVersion $ocrRuntimeVersion -Architecture $Architecture `
    -ArchivePath $runtimeArchivePath -UploadUrl $ocrRuntimeUrl -Files $runtimeFiles `
    -NativeProbePassed $ocrRuntimeNativeProbePassed |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $runtimeReleaseManifest -Encoding utf8

$modelDescriptors = @()
foreach ($model in $ocrModels) {
    $modelCache = Join-Path $artifactRoot "ocr-models-$($model.Id)"
    New-Item -ItemType Directory -Path $modelCache -Force | Out-Null
    $files = @()
    foreach ($file in $model.Files) {
        $path = Join-Path $modelCache $file.Name
        $url = "$ocrModelRootUrl/$($model.Directory)/$($file.Name)"
        $valid = $false
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            try {
                Assert-ReleaseFile -Path $path -Bytes $file.Bytes -Sha256 $file.Sha256
                $valid = $true
            }
            catch {
                Remove-Item -LiteralPath $path -Force
            }
        }
        if (-not $valid) {
            Write-Host "Downloading OCR model: $url"
            Invoke-WebRequest -Uri $url -OutFile $path -MaximumRedirection 5
            Assert-ReleaseFile -Path $path -Bytes $file.Bytes -Sha256 $file.Sha256
        }
        $files += [ordered]@{
            name = $file.Name
            size = $file.Bytes
            sha256 = $file.Sha256
            url = $url
        }
    }
    $null = Invoke-ClipOcrModelSetValidation -Architecture $Architecture `
        -Executable (Join-Path $runtimeWork $ocrRuntimeFileName) `
        -Detector (Join-Path $modelCache $model.Detector) `
        -Recognizer (Join-Path $modelCache $model.Recognizer) `
        -Dictionary (Join-Path $modelCache $model.Dictionary)
    $modelDescriptors += [ordered]@{
        type = $model.Type
        id = $model.Id
        detector = $model.Detector
        recognizer = $model.Recognizer
        dictionary = $model.Dictionary
        files = $files
    }
}

$assetManifest = [ordered]@{
    schema = 2
    default_model = $ocrDefaultModelType
    runtime = [ordered]@{
        version = $ocrRuntimeVersion
        platform = $ocrPlatform
        archive = $runtimeArchive
        files = $runtimeFiles
    }
    models = $modelDescriptors
}
$assetManifestSource = Join-Path $artifactRoot "clip-shot-ocr-asset-manifest.json"
$assetManifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $assetManifestSource -Encoding utf8

# Development builds stage their OCR payload from a checked-in copy of this
# manifest (see cmake/StageClipShotOcrAssets.cmake). It must describe exactly
# the payload this script produces and publishes, so reject drift here instead
# of letting development trees stage assets the release no longer matches.
$checkedInManifestPath = $ocrAssetManifestPath
if (-not (Test-Path -LiteralPath $checkedInManifestPath -PathType Leaf)) {
    throw "The checked-in OCR asset manifest is missing: $checkedInManifestPath"
}
$checkedInManifest = Get-Content -LiteralPath $checkedInManifestPath -Raw | ConvertFrom-Json

function Compare-OcrAssetFileList {
    param(
        [Parameter(Mandatory = $true)]$CheckedIn,
        [Parameter(Mandatory = $true)]$Generated,
        [Parameter(Mandatory = $true)][string]$Component,
        [Parameter(Mandatory = $true)][bool]$RequireUrl
    )
    if ($CheckedIn.Count -ne $Generated.Count) {
        return "$Component file count drifted ($($CheckedIn.Count) checked in, $($Generated.Count) generated)"
    }
    for ($index = 0; $index -lt $CheckedIn.Count; $index++) {
        foreach ($field in "name", "size", "sha256") {
            if ("$($CheckedIn[$index].$field)" -ne "$($Generated[$index].$field)") {
                return "$Component file '$($CheckedIn[$index].name)' $field drifted " +
                       "('$($CheckedIn[$index].$field)' checked in, '$($Generated[$index].$field)' generated)"
            }
        }
        if ($RequireUrl -and "$($CheckedIn[$index].url)" -ne "$($Generated[$index].url)") {
            return "$Component file '$($CheckedIn[$index].name)' url drifted"
        }
    }
    return $null
}

$manifestDrift = $null
if ($checkedInManifest.schema -ne $assetManifest.schema) {
    $manifestDrift = "schema drifted"
}
elseif ($checkedInManifest.default_model -ne $assetManifest.default_model) {
    $manifestDrift = "default model drifted"
}
elseif ($checkedInManifest.runtime.version -ne $assetManifest.runtime.version -or
        $checkedInManifest.runtime.platform -ne $assetManifest.runtime.platform) {
    $manifestDrift = "runtime version/platform drifted"
}
else {
    foreach ($field in "name", "size", "sha256", "url") {
        if ("$($checkedInManifest.runtime.archive.$field)" -ne "$($assetManifest.runtime.archive.$field)") {
            $manifestDrift = "runtime archive $field drifted"
            break
        }
    }
}
if (-not $manifestDrift) {
    $manifestDrift = Compare-OcrAssetFileList `
        -CheckedIn $checkedInManifest.runtime.files -Generated $assetManifest.runtime.files `
        -Component "runtime" -RequireUrl $false
}
if (-not $manifestDrift -and $checkedInManifest.models.Count -ne $assetManifest.models.Count) {
    $manifestDrift = "model count drifted"
}
if (-not $manifestDrift) {
    for ($modelIndex = 0; $modelIndex -lt $assetManifest.models.Count; $modelIndex++) {
        $checkedModel = $checkedInManifest.models[$modelIndex]
        $generatedModel = $assetManifest.models[$modelIndex]
        foreach ($field in "type", "id", "detector", "recognizer", "dictionary") {
            if ("$($checkedModel.$field)" -ne "$($generatedModel.$field)") {
                $manifestDrift = "model '$($generatedModel.type)' $field drifted"
                break
            }
        }
        if ($manifestDrift) { break }
        $manifestDrift = Compare-OcrAssetFileList `
            -CheckedIn $checkedModel.files -Generated $generatedModel.files `
            -Component "model '$($generatedModel.type)'" -RequireUrl $true
        if ($manifestDrift) { break }
    }
}
if ($manifestDrift) {
    throw "The checked-in OCR asset manifest ($checkedInManifestPath) no longer matches the " +
          "packaged payload: $manifestDrift. If the OCR runtime or models changed intentionally, " +
          "bump the OCR runtime version, publish the new payload, and update the checked-in manifest " +
          "in the same change."
}

# Only a verified download proves publication. A supplied native archive may be
# staged for offline validation before its separate immutable OCR publication.
if (-not $OcrRuntimeArchive) {
    $runtimeArchive.sha256 | Set-Content -LiteralPath $runtimePublishedMarker -Encoding ascii
}

function New-ClipPortableLauncher {
    param([Parameter(Mandatory)][string]$StageRoot)
    # A relative-path .lnk cannot be produced reliably on all runners (the
    # ShellLink COM object on this environment does not expose IPersistFile),
    # so the portable root ships a VBS launcher: it resolves its own directory
    # at run time, so the whole folder stays movable and works from any
    # extraction location without a black console window.
    $launcherPath = Join-Path $StageRoot 'Clip.vbs'
    $vbs = @'
Set fso = CreateObject("Scripting.FileSystemObject")
Set ws = CreateObject("WScript.Shell")
dir = fso.GetParentFolderName(WScript.ScriptFullName)
ws.Run """" & dir & "\bin\clip_shot.exe""", 1, False
'@
    Set-Content -LiteralPath $launcherPath -Value $vbs -Encoding ASCII
    if (-not (Test-Path -LiteralPath $launcherPath -PathType Leaf)) {
        throw "Failed to create the portable root launcher: $launcherPath"
    }
}

$portableStage = Join-Path $artifactRoot "clip-shot-$packageVersion-portable-stage"
$defaultOcrModel = @($ocrModels | Where-Object { $_.Type -eq $ocrDefaultModelType })[0]
$defaultModelCache = Join-Path $artifactRoot "ocr-models-$($defaultOcrModel.Id)"
Reset-ReleaseDirectory -Path $portableStage
Copy-Item -Path (Join-Path $installDirectory "*") -Destination $portableStage -Recurse -Force
Remove-Item -LiteralPath (Join-Path $portableStage "bin\clip-ocr-process.exe") -Force
Remove-Item -LiteralPath (Join-Path $portableStage "bin\DirectML.dll") -Force
$assetRoot = Join-Path $portableStage "bin\assets\ocr"
Reset-ReleaseDirectory -Path $assetRoot
Copy-Item -LiteralPath $assetManifestSource -Destination (Join-Path $assetRoot "asset-manifest.json")
$runtimeDestination = Join-Path $assetRoot "runtimes\$ocrRuntimeVersion\$ocrPlatform"
$modelDestination = Join-Path $assetRoot "models\$ocrDefaultModelId"
New-Item -ItemType Directory -Path $runtimeDestination, $modelDestination -Force | Out-Null
Copy-Item -Path (Join-Path $runtimeWork "*") -Destination $runtimeDestination -Force
foreach ($file in $defaultOcrModel.Files) {
    Copy-Item -LiteralPath (Join-Path $defaultModelCache $file.Name) -Destination $modelDestination
}
[ordered]@{ schema = 1; component = $ocrRuntimeVersion } |
    ConvertTo-Json -Compress | Set-Content -LiteralPath (Join-Path $runtimeDestination ".complete.json") -Encoding utf8
[ordered]@{ schema = 1; component = $ocrDefaultModelId } |
    ConvertTo-Json -Compress | Set-Content -LiteralPath (Join-Path $modelDestination ".complete.json") -Encoding utf8
"portable" | Set-Content -LiteralPath (Join-Path $portableStage "bin\__data_directory") `
    -Encoding ascii -NoNewline
New-ClipPortableLauncher -StageRoot $portableStage
Assert-ClipArm64StackCookieSafety -Path $portableStage -BuildDirectory $buildDirectory -Architecture $Architecture
$owned = @(Get-ReleaseTreeFileManifest -Root $portableStage | ForEach-Object {
    [ordered]@{ path = $_.Path.Replace('\', '/'); size = $_.Bytes; sha256 = $_.Sha256 }
})
[ordered]@{ schema = 1; platform = $ocrPlatform; version = $packageVersion; variant = 'portable'; files = $owned } |
    ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $portableStage 'clip-shot-installation.json') -Encoding utf8NoBOM

$portableOcrRoot = Join-Path $portableStage "bin\assets\ocr"
$expectedPortableDirectories = @(
    "models",
    "models\$ocrDefaultModelId",
    "runtimes",
    "runtimes\$ocrRuntimeVersion",
    "runtimes\$ocrRuntimeVersion\$ocrPlatform"
) | Sort-Object
$actualPortableDirectories = @(Get-ChildItem -LiteralPath $portableOcrRoot -Recurse -Directory |
    ForEach-Object { [System.IO.Path]::GetRelativePath($portableOcrRoot, $_.FullName) } |
    Sort-Object)
$expectedPortableFiles = @(
    "asset-manifest.json",
    "models\$ocrDefaultModelId\.complete.json",
    "models\$ocrDefaultModelId\$($defaultOcrModel.Detector)",
    "models\$ocrDefaultModelId\$($defaultOcrModel.Recognizer)",
    "models\$ocrDefaultModelId\$($defaultOcrModel.Dictionary)",
    "runtimes\$ocrRuntimeVersion\$ocrPlatform\.complete.json",
    "runtimes\$ocrRuntimeVersion\$ocrPlatform\DirectML.dll",
    "runtimes\$ocrRuntimeVersion\$ocrPlatform\runtime-manifest.json",
    "runtimes\$ocrRuntimeVersion\$ocrPlatform\$ocrRuntimeFileName"
) | Sort-Object
$actualPortableFiles = @(Get-ChildItem -LiteralPath $portableOcrRoot -Recurse -File |
    ForEach-Object { [System.IO.Path]::GetRelativePath($portableOcrRoot, $_.FullName) } |
    Sort-Object)
if (@(Compare-Object $expectedPortableDirectories $actualPortableDirectories).Count -ne 0 -or
    @(Compare-Object $expectedPortableFiles $actualPortableFiles).Count -ne 0) {
    throw "The portable package must contain exactly runtime $ocrRuntimeVersion and the Small OCR model."
}
$portableDataMarker = Join-Path $portableStage "bin\__data_directory"
if (-not (Test-Path -LiteralPath $portableDataMarker -PathType Leaf) -or
    (Get-Content -LiteralPath $portableDataMarker -Raw) -cne "portable") {
    throw "The portable package stage is missing its portable data-directory marker."
}

$producedPackages = @()
Assert-ClipNativeStartup -Stage $portableStage -Executable clip_shot -Updater clip-shot-updater -Version $packageVersion
$nativeProbesPassed = $nativeTarget

$portableBaseName = "clip-shot-$packageVersion-$ocrPlatform-portable"
$portableArchivePath = Join-Path $buildDirectory "$portableBaseName.zip"
New-DeterministicZip -SourceDirectory $portableStage -Destination $portableArchivePath
$portableArchiveHash =
    (Get-FileHash -LiteralPath $portableArchivePath -Algorithm SHA256).Hash.ToLowerInvariant()
$portableChecksumPath = "$portableArchivePath.sha256"
"$portableArchiveHash  $portableBaseName.zip" |
    Set-Content -LiteralPath $portableChecksumPath -Encoding ascii
$portableStageFileManifest = Get-ReleaseTreeFileManifest -Root $portableStage
Assert-ZipMatchesFileManifest -ArchivePath $portableArchivePath `
    -FileManifest $portableStageFileManifest
$portableManifestPath = Join-Path $buildDirectory "$portableBaseName.manifest.json"
[ordered]@{
    SchemaVersion = 2
    PackageVersion = $packageVersion
    Variant = "portable"
    Preset = $releasePreset
    Architecture = $Architecture
    Platform = $ocrPlatform
    NativeValidationRequired = (-not $nativeTarget)
    NativeValidation = (New-ClipNativeValidation $portableArchivePath)
    StaticCrt = $true
    StaticQt = $true
    StaticImageCodecBackend = $true
    OcrRuntimeVersion = $ocrRuntimeVersion
    OcrDefaultModelType = $ocrDefaultModelType
    OcrDefaultModelId = $ocrDefaultModelId
    Qt = $qtStamp
    InstallTreeBytes = [long](($portableStageFileManifest | Measure-Object -Property Bytes -Sum).Sum)
    InstallFiles = $portableStageFileManifest
    Archive = [ordered]@{
        Path = (Split-Path -Leaf $portableArchivePath)
        Bytes = (Get-Item -LiteralPath $portableArchivePath).Length
        Sha256 = $portableArchiveHash
    }
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $portableManifestPath -Encoding utf8
$producedPackages += $portableArchivePath
Write-Output "Clip portable archive: $portableArchivePath"
Write-Output "Clip portable archive checksum: $portableChecksumPath"
Write-Output "Clip portable release manifest: $portableManifestPath"
Write-Output "Clip audited install tree: $installDirectory"
Write-Output "OCR runtime upload artifact: $runtimeArchivePath"
Write-Output "OCR runtime checksum: $runtimeArchiveChecksum"
Write-Output "OCR runtime manifest: $runtimeReleaseManifest"

. (Join-Path $PSScriptRoot "package-clip-shot-mini.ps1")
