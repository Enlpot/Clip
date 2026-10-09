[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $PSScriptRoot "clip-build-environment.ps1")

function Require([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Require-Arguments([string[]]$Actual, [string[]]$Expected, [string]$Message) {
    Require (($Actual -join "`n") -ceq ($Expected -join "`n")) $Message
}

$testRoot = Join-Path $repoRoot ("build/windows-release-workflow-tests-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
try {
    $customBuild = Join-Path $testRoot "custom-release"
    New-Item -ItemType Directory -Path $customBuild | Out-Null
    $expectedConfigure = @("--preset", "clip-shot-msvc-release", "-S", $repoRoot,
        "-B", $customBuild, "-DVCPKG_HOST_TRIPLET=$((Get-ClipWindowsTarget).HostTriplet)",
        '-DCLIP_RUST_TARGET=x86_64-pc-windows-msvc')
    if ($env:LIBCLANG_PATH) {
        $expectedConfigure += "-DCLIP_LIBCLANG_BIN_DIR:PATH=$env:LIBCLANG_PATH"
        $expectedConfigure += "-DCLIP_SHOT_LIBCLANG_DIR:PATH=$env:LIBCLANG_PATH"
    }
    if ((Get-ClipWindowsHostArchitecture) -ceq 'x64') { $expectedConfigure += '-UQT_HOST_PATH' }
    else { $expectedConfigure += "-DQT_HOST_PATH:PATH=$env:QT_HOST_PATH" }

    $missingCacheArguments = @(Get-ClipConfigureArguments -Preset clip-shot-msvc-release `
        -BuildDirectory $customBuild)
    Require-Arguments $missingCacheArguments $expectedConfigure "A new build must use the requested directory."

    Set-Content -LiteralPath (Join-Path $customBuild "CMakeCache.txt") -Value "fixture" -Encoding utf8
    function Test-ClipCacheAlignment {
        param([string]$CachePath, [string]$Preset)
        return $script:CacheAligned
    }
    $script:CacheAligned = $true
    $alignedArguments = @(Get-ClipConfigureArguments -Preset clip-shot-msvc-release `
        -BuildDirectory $customBuild)
    Require-Arguments $alignedArguments $expectedConfigure "An aligned cache must be reused."

    $script:CacheAligned = $false
    $unalignedArguments = @(Get-ClipConfigureArguments -Preset clip-shot-msvc-release `
        -BuildDirectory $customBuild)
    Require-Arguments $unalignedArguments (@("--fresh") + $expectedConfigure) `
        "An unaligned cache must be replaced."

    $presetData = Get-Content -LiteralPath (Join-Path $repoRoot "CMakePresets.json") -Raw |
        ConvertFrom-Json
    foreach ($name in @("build-clip-shot-msvc-release", "build-clip-shot-msvc-fast")) {
        $preset = @($presetData.buildPresets | Where-Object { $_.name -ceq $name })
        Require ($preset.Count -eq 1 -and @($preset[0].targets).Count -eq 2 -and
            $preset[0].targets[0] -ceq "clip_shot" -and $preset[0].targets[1] -ceq "clip_shot_mini") "$name must build both Clip editions by default."
    }

    $fixtureScripts = Join-Path $testRoot "scripts"
    New-Item -ItemType Directory -Path $fixtureScripts | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'stage-clip-shot-release.ps1') -Destination $fixtureScripts
    Set-Content -LiteralPath (Join-Path $fixtureScripts 'package-clip-shot.ps1') -Value @'
param([string]$Architecture, [string]$BuildDirectory, [string]$InstallDirectory,
    [int]$Parallelism, [string]$OcrRuntimeArchive, [switch]$SkipBuild)
$global:ClipStageParameters = $PSBoundParameters
$global:LASTEXITCODE = 0
'@
    & (Join-Path $fixtureScripts 'stage-clip-shot-release.ps1') -Architecture arm64 -SkipBuild
    Require ($global:ClipStageParameters.Architecture -ceq 'arm64' -and
        $global:ClipStageParameters.BuildDirectory -ceq 'build/clip-shot-msvc-arm64-release' -and
        $global:ClipStageParameters.InstallDirectory -ceq 'artifacts/windows-arm64/clip-shot' -and
        $global:ClipStageParameters.SkipBuild) 'ARM release staging must stay inside its architecture artifact root.'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "build.ps1") -Destination $fixtureScripts
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'run-clip-shot.ps1') -Destination $fixtureScripts
    @'
function Set-ClipBuildEnvironment { param([string]$Preset) return $null }
function Get-ClipWindowsTarget {
    param([string]$Preset)
    return [pscustomobject]@{ IsStatic = ($Preset -match '(release|fast)$'); HostArchitecture = 'x64'
        Configuration = $(if ($Preset -match '-debug$') { 'Debug' } else { 'Release' })
        Architecture = $(if ($Preset -match '-arm64-') { 'arm64' } else { 'x64' }) }
}
function Get-ClipConfigureArguments {
    param([string]$Preset, [string]$BuildDirectory)
    return @("--preset", $Preset, "-B", $BuildDirectory)
}
'@ | Set-Content -LiteralPath (Join-Path $fixtureScripts "clip-build-environment.ps1") -Encoding utf8
    @'
param([switch]$SkipDependencyInstall, [string[]]$VcpkgVariants, [string]$Architecture, [string]$Preset)
$global:ClipBootstrapVariants = @($VcpkgVariants)
$global:LASTEXITCODE = 0
'@ | Set-Content -LiteralPath (Join-Path $fixtureScripts "bootstrap.ps1") -Encoding utf8

    function global:cmake {
        $global:ClipBuildCalls += ,@($args)
        if ($args -contains '-B') {
            $configuredBuild = $args[[Array]::IndexOf($args, '-B') + 1]
            $null = New-Item -ItemType Directory -Path $configuredBuild -Force
            "CLIP_APPS_BUILD_CLIP_SHOT_MINI:BOOL=$global:ClipMiniEnabled" |
                Set-Content -LiteralPath (Join-Path $configuredBuild 'CMakeCache.txt')
        }
        $global:LASTEXITCODE = 0
    }
    $global:ClipMiniEnabled = 'ON'
    $launchRejected = $false
    try { & (Join-Path $fixtureScripts 'run-clip-shot.ps1') -Preset clip-shot-msvc-arm64-fast -NoBuild }
    catch { $launchRejected = $_.Exception.Message -match 'requires a Windows ARM64 host' }
    Require $launchRejected 'Cross-built ARM64 launchers must reject x64 hosts before stopping or building applications.'
    $global:ClipBuildCalls = @()
    $global:ClipBootstrapVariants = @()
    & (Join-Path $fixtureScripts "build.ps1") -Preset clip-shot-msvc-release
    Require-Arguments $global:ClipBootstrapVariants @("Static") `
        "Release builds must bootstrap only static host tools."
    Require ($global:ClipBuildCalls.Count -eq 2) "Release build must configure and build once."
    Require-Arguments $global:ClipBuildCalls[1] `
        @("--build", "--preset", "build-clip-shot-msvc-release", "--parallel") `
        "Release build must use the preset's Clip target."

    $global:ClipMiniEnabled = 'OFF'
    $global:ClipBuildCalls = @()
    & (Join-Path $fixtureScripts "build.ps1") -Preset clip-shot-msvc-release -SkipBootstrap
    Require-Arguments $global:ClipBuildCalls[1] `
        @('--build', '--preset', 'build-clip-shot-msvc-release', '--parallel', '--target', 'clip_shot') `
        'Default release wrapper must respect an explicitly disabled Mini target.'

    $global:ClipBuildCalls = @()
    & (Join-Path $fixtureScripts "build.ps1") -Preset windows-msvc-debug -Target clip_shot
    Require-Arguments $global:ClipBootstrapVariants @("Dynamic") `
        "Debug builds must bootstrap only dynamic host tools."
    Require-Arguments $global:ClipBuildCalls[1] `
        @("--build", "--preset", "build-windows-msvc-debug", "--parallel", "--target", "clip_shot") `
        "An explicit target must override the preset default."
}
finally {
    Remove-Item Function:\cmake -ErrorAction SilentlyContinue
    Remove-Variable ClipBuildCalls, ClipBootstrapVariants, ClipMiniEnabled -Scope Global -ErrorAction SilentlyContinue
    Remove-Variable ClipStageParameters -Scope Global -ErrorAction SilentlyContinue
    $resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
    $resolvedBuildRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot "build"))
    if (-not $resolvedTestRoot.StartsWith($resolvedBuildRoot.TrimEnd('\') + '\',
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a test directory outside $resolvedBuildRoot"
    }
    Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
}

Write-Output "Windows release workflow tests passed."
