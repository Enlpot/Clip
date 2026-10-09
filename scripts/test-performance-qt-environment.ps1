#requires -Version 7.2
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $repoRoot "clip_shot/scripts/performance-environment.ps1")

function Require([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Require-Rejected([scriptblock]$Action, [string]$Message) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true }
    Require $rejected $Message
}

function Write-PeFixture([string]$Path, [int]$Machine) {
    $bytes = [byte[]]::new(128)
    $bytes[0] = 0x4D; $bytes[1] = 0x5A; $bytes[0x3C] = 64
    $bytes[64] = 0x50; $bytes[65] = 0x45
    [BitConverter]::GetBytes([uint16]$Machine).CopyTo($bytes, 68)
    [IO.File]::WriteAllBytes($Path, $bytes)
}

$environmentNames = @("PATH", "QTDIR", "Qt6_DIR", "QT_QPA_PLATFORM", "QT_QPA_PLATFORM_PLUGIN_PATH")
$savedEnvironment = @{}
foreach ($name in $environmentNames) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, "Process")
}
$testRoot = Join-Path $repoRoot ("build/performance-qt-contract-" + [guid]::NewGuid().ToString("N"))
try {
    $script:ClipPerformanceFixtureHost = 'x64'
    function Get-ClipWindowsHostArchitecture { return $script:ClipPerformanceFixtureHost }
    Require ((Get-ClipPerformanceTarget).Preset -ceq 'windows-msvc-performance') `
        'The default performance preset must retain its x64 identity.'
    $armTarget = Get-ClipPerformanceTarget -Architecture arm64
    Require ($armTarget.Preset -ceq 'clip-shot-msvc-arm64-performance' -and
        $armTarget.Configuration -ceq 'Release' -and -not $armTarget.IsStatic) `
        'ARM64 benchmarks must use the dedicated Release performance preset.'
    Require-Rejected { Get-ClipPerformanceTarget -Architecture arm64 -RequireNative } `
        'Cross builds may configure benchmarks but cannot execute them.'
    $kit = Join-Path $testRoot "Qt kit"
    $qtDir = Join-Path $kit "lib/cmake/Qt6"
    $coreDir = Join-Path $kit "lib/cmake/Qt6Core"
    $bin = Join-Path $kit "bin"
    New-Item -ItemType Directory -Path $qtDir, $coreDir, $bin -Force | Out-Null
    $coreTargets = Join-Path $coreDir "Qt6CoreTargets.cmake"
    $env:Qt6_DIR = $qtDir
    $env:QTDIR = $kit
    $env:QT_QPA_PLATFORM = "original-platform"
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = "original-plugin-path"
    $originalPath = $env:PATH

    Set-Content -LiteralPath $coreTargets -Value 'QT_ENABLED_PUBLIC_FEATURES "static;timezone"'
    $snapshot = Set-ClipPerformanceQtRuntime -Platform offscreen
    Require ($env:QT_QPA_PLATFORM -ceq "offscreen") "Static replay must select offscreen QPA."
    Require ([string]::IsNullOrEmpty($env:QT_QPA_PLATFORM_PLUGIN_PATH)) `
        "Static Qt must not inherit a stale external platform plugin directory."
    Require (-not (Test-Path -LiteralPath (Join-Path $bin "Qt6Core.dll"))) `
        "The static fixture must work without any Qt DLL."
    Restore-ClipPerformanceQtRuntime -Snapshot $snapshot
    Require ($env:PATH -ceq $originalPath -and
        $env:QT_QPA_PLATFORM -ceq "original-platform" -and
        $env:QT_QPA_PLATFORM_PLUGIN_PATH -ceq "original-plugin-path") `
        "Runtime cleanup must restore the caller's environment."

    Set-Content -LiteralPath $coreTargets -Value 'QT_ENABLED_PUBLIC_FEATURES "shared;timezone"'
    $rejected = $false
    try { Set-ClipPerformanceQtRuntime | Out-Null }
    catch { $rejected = $_.Exception.Message -match "selected shared Qt $script:ClipQtVersion runtime" }
    Require $rejected "A shared kit without its runtime DLL must fail before launching benchmarks."
    Require ($env:PATH -ceq $originalPath -and $env:QT_QPA_PLATFORM -ceq "original-platform") `
        "Failed shared-kit validation must preserve the environment."
    Write-PeFixture (Join-Path $bin 'Qt6Core.dll') 0x8664
    $snapshot = Set-ClipPerformanceQtRuntime
    Require ($env:QT_QPA_PLATFORM -ceq "windows" -and
        $env:QT_QPA_PLATFORM_PLUGIN_PATH -ceq (Join-Path $kit "plugins/platforms")) `
        "Shared native benchmarks must use the selected kit's platform plugins."
    Restore-ClipPerformanceQtRuntime -Snapshot $snapshot
    Write-PeFixture (Join-Path $bin 'Qt6Core.dll') 0xAA64
    Require-Rejected { Set-ClipPerformanceQtRuntime } 'Wrong-machine shared Qt cannot run a benchmark.'
    Require ($env:PATH -ceq $originalPath) 'Wrong-machine runtime validation must preserve PATH.'
    $script:ClipPerformanceFixtureHost = 'arm64'
    $snapshot = Set-ClipPerformanceQtRuntime -Architecture arm64
    Require ($env:PATH -like "*$($armTarget.InstalledRoot)*arm64-windows*bin*") `
        'Native ARM64 benchmarks must load dependencies from the ARM64 triplet.'
    Restore-ClipPerformanceQtRuntime -Snapshot $snapshot
    $benchmark = Join-Path $testRoot 'benchmark.exe'
    Write-PeFixture $benchmark 0xAA64
    Assert-ClipPerformanceExecutable -Path $benchmark -Architecture arm64
    Write-PeFixture $benchmark 0x8664
    Require-Rejected { Assert-ClipPerformanceExecutable -Path $benchmark -Architecture arm64 } `
        'Native ARM64 must reject an emulated x64 benchmark.'
    $script:ClipPerformanceFixtureHost = 'x64'

    function Set-ClipQtEnvironment {
        param([string]$Qt6Dir, [string]$Preset)
        $global:ClipPerformanceFixtureQtDir = $Qt6Dir
        $global:ClipPerformanceFixtureQtPreset = $Preset
    }
    function Set-ClipBuildEnvironment {
        param([string]$Preset)
        $global:ClipPerformanceFixtureBuildPreset = $Preset
    }
    Initialize-ClipPerformanceEnvironment -QtBin $bin
    Require ($global:ClipPerformanceFixtureQtDir -ceq $qtDir -and
        $global:ClipPerformanceFixtureQtPreset -ceq "windows-msvc-performance" -and
        $global:ClipPerformanceFixtureBuildPreset -ceq "windows-msvc-performance") `
        "An explicit Qt bin must select the same kit for the Release performance build."
    Initialize-ClipPerformanceEnvironment -QtBin $bin -Architecture arm64
    Require ($global:ClipPerformanceFixtureQtPreset -ceq 'clip-shot-msvc-arm64-performance' -and
        $global:ClipPerformanceFixtureBuildPreset -ceq 'clip-shot-msvc-arm64-performance') `
        'ARM64 configure must select the target kit and performance preset consistently.'

    # Exercise the actual configure wrapper with stubbed toolchain/native calls.
    # No compiler, network, Rust run, benchmark, or interactive window is started.
    $fixtureScripts = Join-Path $testRoot "workspace/clip_shot/scripts"
    New-Item -ItemType Directory -Path $fixtureScripts -Force | Out-Null
    $configure = Join-Path $fixtureScripts "configure-msvc-perf.ps1"
    Copy-Item -LiteralPath (Join-Path $repoRoot "clip_shot/scripts/configure-msvc-perf.ps1") `
        -Destination $configure
    Set-Content -LiteralPath (Join-Path $fixtureScripts "performance-environment.ps1") -Value @'
function Initialize-ClipPerformanceEnvironment {
    param([string]$QtBin, [string]$Architecture)
    $global:ClipPerformanceFixtureInitialized = $QtBin
    $global:ClipPerformanceFixtureArchitecture = $Architecture
}
function Get-ClipPerformanceTarget {
    param([string]$Architecture)
    return [pscustomobject]@{ Preset = $(if ($Architecture -eq 'arm64') { 'clip-shot-msvc-arm64-performance' } else { 'windows-msvc-performance' }) }
}
function Get-ClipConfigureArguments {
    param([string]$Preset, [string]$BuildDirectory)
    $arguments = @("--preset", $Preset, "-B", $BuildDirectory)
    if ($global:ClipPerformanceFixtureAlignedFresh) { $arguments = @("--fresh") + $arguments }
    return $arguments
}
'@
    function cmake {
        $global:ClipPerformanceFixtureArguments = @($args)
        $global:LASTEXITCODE = 0
    }
    foreach ($architecture in @('x64', 'arm64')) {
      foreach ($alreadyFresh in @($false, $true)) {
        $global:ClipPerformanceFixtureAlignedFresh = $alreadyFresh
        & $configure -Fresh -QtBin $bin -Architecture $architecture
        Require ($global:ClipPerformanceFixtureInitialized -ceq $bin) `
            "Configure must initialize the selected toolchain in the current PowerShell host."
        Require (@($global:ClipPerformanceFixtureArguments | Where-Object { $_ -ceq "--fresh" }).Count -eq 1) `
            "Explicit Fresh and automatic cache alignment must not duplicate --fresh."
        $expectedPreset = if ($architecture -eq 'arm64') { 'clip-shot-msvc-arm64-performance' } else { 'windows-msvc-performance' }
        Require ($global:ClipPerformanceFixtureArguments -contains $expectedPreset -and
            $global:ClipPerformanceFixtureArguments -contains (Join-Path (Split-Path -Parent (Split-Path -Parent $fixtureScripts)) "build/$expectedPreset") -and
            $global:ClipPerformanceFixtureArchitecture -ceq $architecture) `
            "Benchmarks must configure the dedicated Release performance preset."
      }
    }

    foreach ($name in @("configure-msvc-perf", "performance-environment", "run-scrolling-perf",
            "run-toolbar-perf", "run-toolbar-display-perf", "run-pin-to-screen-perf",
            "run-capture-startup-perf", "run-file-pin-batch-perf", "run-pinned-lifecycle-perf",
            "run-recording-window-startup-perf", "run-uia-perf")) {
        $path = Join-Path $repoRoot "clip_shot/scripts/$name.ps1"
        $errors = $null
        $ast = [System.Management.Automation.Language.Parser]::ParseFile($path, [ref]$null, [ref]$errors)
        Require ($errors.Count -eq 0) "$name must parse successfully."
        $legacyHost = $ast.Find({
            param($node)
            $node -is [System.Management.Automation.Language.CommandAst] -and
                $node.GetCommandName() -eq "powershell"
        }, $true)
        Require ($null -eq $legacyHost) "$name must not launch Windows PowerShell 5."
        if ($name -like 'run-*-perf') {
            $nativeGuard = $ast.Find({ param($node)
                $node -is [System.Management.Automation.Language.CommandAst] -and
                $node.GetCommandName() -ceq 'Get-ClipPerformanceTarget' -and
                $node.CommandElements.Extent.Text -ccontains '-RequireNative'
            }, $true)
            Require ($null -ne $nativeGuard) "$name must check the native host before running benchmarks."
            $opposite = if ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() -ceq 'Arm64') { 'x64' } else { 'arm64' }
            $fixtureArguments = @{ Architecture = $opposite }
            if ($name -ceq 'run-uia-perf') {
                $fixtureArguments.BeforeExecutable = $benchmark; $fixtureArguments.AfterExecutable = $benchmark
                $fixtureArguments.WindowHandle = 1; $fixtureArguments.Points = @(0, 0)
            }
            $rejected = $false
            try { & $path @fixtureArguments } catch { $rejected = $_.Exception.Message -like 'Performance benchmarks require a native*' }
            Require $rejected "$name must reject a cross-host run before configuring, building, or executing."
        }
    }
}
finally {
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], "Process")
    }
    if (Test-Path -LiteralPath $testRoot) {
        $resolvedRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "build")) + [IO.Path]::DirectorySeparatorChar
        $resolvedTarget = (Resolve-Path -LiteralPath $testRoot).Path
        if (-not $resolvedTarget.StartsWith($resolvedRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove a performance fixture outside the build directory."
        }
        Remove-Item -LiteralPath $resolvedTarget -Recurse -Force
    }
}
Write-Output "Performance Qt runtime and configure contract tests passed."
