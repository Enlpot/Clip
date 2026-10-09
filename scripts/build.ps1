[CmdletBinding()]
param(
    [ValidateSet(
        "windows-msvc-debug",
        "windows-msvc-performance",
        "windows-clang-portability",
        "clip-shot-msvc-release",
        "clip-shot-msvc-fast",
        "clip-shot-msvc-arm64-debug",
        "clip-shot-msvc-arm64-performance",
        "clip-shot-msvc-arm64-release",
        "clip-shot-msvc-arm64-fast"
    )]
    [string]$Preset = "windows-msvc-debug",
    [string]$Target = "",
    [switch]$Clean,
    [switch]$SkipBootstrap
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "clip-build-environment.ps1")
$windowsTarget = Get-ClipWindowsTarget -Preset $Preset
Set-ClipBuildEnvironment -Preset $Preset | Out-Null
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDirectory = Join-Path $repoRoot "build/$Preset"

if ($Clean -and (Test-Path -LiteralPath $buildDirectory)) {
    $resolvedBuild = (Resolve-Path -LiteralPath $buildDirectory).Path
    $resolvedRoot = (Resolve-Path -LiteralPath (Join-Path $repoRoot "build")).Path
    if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a build directory outside $resolvedRoot"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

if (-not $SkipBootstrap) {
    $vcpkgVariant = if ($windowsTarget.IsStatic) {
        "Static"
    }
    else {
        "Dynamic"
    }
    & (Join-Path $PSScriptRoot "bootstrap.ps1") -SkipDependencyInstall -VcpkgVariants $vcpkgVariant `
        -Architecture $windowsTarget.Architecture -Preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "Build environment bootstrap failed."
    }
}

Push-Location $repoRoot
try {
    $configureArguments = @(Get-ClipConfigureArguments -Preset $Preset -BuildDirectory $buildDirectory)
    & cmake @configureArguments
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed for preset $Preset."
    }

    $buildArguments = @("--build", "--preset", "build-$Preset", "--parallel")
    if (-not [string]::IsNullOrWhiteSpace($Target)) {
        $buildArguments += @("--target", $Target)
    }
    elseif ($windowsTarget.IsStatic -and
        -not (Select-String -LiteralPath (Join-Path $buildDirectory 'CMakeCache.txt') `
            -Pattern '^CLIP_APPS_BUILD_CLIP_SHOT_MINI:BOOL=(ON|TRUE|1)$' -Quiet)) {
        $buildArguments += @('--target', 'clip_shot')
    }
    & cmake @buildArguments
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed for preset $Preset."
    }
}
finally {
    Pop-Location
}
