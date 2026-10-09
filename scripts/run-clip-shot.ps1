[CmdletBinding()]
param(
    [ValidateSet(
        "windows-msvc-debug",
        "windows-msvc-performance",
        "clip-shot-msvc-release",
        "clip-shot-msvc-fast",
        "clip-shot-msvc-arm64-debug",
        "clip-shot-msvc-arm64-performance",
        "clip-shot-msvc-arm64-release",
        "clip-shot-msvc-arm64-fast"
    )]
    [string]$Preset = "windows-msvc-debug",
    [ValidateSet("Full", "Mini")][string]$Edition = "Full",
    [switch]$Clean,
    [switch]$NoBuild,
    [switch]$Detached
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
. (Join-Path $PSScriptRoot 'clip-build-environment.ps1')
$windowsTarget = Get-ClipWindowsTarget -Preset $Preset
if ($windowsTarget.Architecture -ceq 'arm64' -and $windowsTarget.HostArchitecture -cne 'arm64') {
    throw 'Running Clip ARM64 requires a Windows ARM64 host.'
}
$target = if ($Edition -eq "Mini") { "clip_shot_mini" } else { "clip_shot" }
$buildDirectory = Join-Path $repoRoot "build\$Preset"
$configuration = $windowsTarget.Configuration
$executablePath = Join-Path $buildDirectory "$target\$configuration\$target.exe"

function Test-PathIsUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory
    )

    $directoryPath = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    $candidatePath = [System.IO.Path]::GetFullPath($Path)

    return $candidatePath.StartsWith(
        $directoryPath,
        [System.StringComparison]::OrdinalIgnoreCase
    )
}

function Stop-RunningBuildInstance {
    param([Parameter(Mandatory = $true)][string]$BuildDirectory)

    $runningProcesses = @(Get-Process -Name $target -ErrorAction SilentlyContinue)
    foreach ($process in $runningProcesses) {
        try {
            $processPath = $process.Path
        }
        catch {
            Write-Verbose "Unable to inspect clip_shot process $($process.Id); leaving it running."
            continue
        }

        if ([string]::IsNullOrWhiteSpace($processPath) -or
            -not (Test-PathIsUnderDirectory -Path $processPath -Directory $BuildDirectory)) {
            continue
        }

        Write-Host "Stopping the running development instance (PID $($process.Id))..."
        try {
            if ($process.CloseMainWindow() -and $process.WaitForExit(1500)) {
                continue
            }
        }
        catch {
            # The process may have exited between discovery and shutdown.
        }

        $process.Refresh()
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            if (-not $process.WaitForExit(5000)) {
                throw "clip_shot process $($process.Id) did not stop; the executable is still locked."
            }
        }
    }
}

if (-not $NoBuild) {
    Stop-RunningBuildInstance -BuildDirectory $buildDirectory

    $buildScript = Join-Path $PSScriptRoot "build.ps1"
    $vcpkgExecutable = Join-Path $repoRoot ".tools\vcpkg\vcpkg.exe"
    $skipBootstrap = Test-Path -LiteralPath $vcpkgExecutable -PathType Leaf
    & $buildScript -Preset $Preset -Target $target -Clean:$Clean -SkipBootstrap:$skipBootstrap
    if ($LASTEXITCODE -ne 0) {
        throw "Clip build failed with exit code $LASTEXITCODE."
    }
}

if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    $buildHint = if ($NoBuild) {
        "Run this script without -NoBuild to create it."
    }
    else {
        "The build completed without producing the expected target."
    }
    throw "Clip executable was not found at '$executablePath'. $buildHint"
}

$workingDirectory = Split-Path -Parent $executablePath
if ((Get-ClipPeMachine -Path $executablePath) -ne $windowsTarget.PeMachine) {
    throw "The Clip executable does not match preset $Preset. Rebuild its selected architecture."
}
if ($Detached) {
    Start-Process -FilePath $executablePath -WorkingDirectory $workingDirectory -WindowStyle Hidden | Out-Null
    return
}

$process = Start-Process -FilePath $executablePath -WorkingDirectory $workingDirectory `
    -WindowStyle Hidden -PassThru
$process.WaitForExit()
if ($process.ExitCode -ne 0) {
    throw "Clip exited with code $($process.ExitCode)."
}
