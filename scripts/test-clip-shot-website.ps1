[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function Require([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
$testRoot = Join-Path ([IO.Path]::GetTempPath()) "clip-website-wrapper-tests-$([guid]::NewGuid().ToString('N'))"
$null = New-Item -ItemType Directory -Path (Join-Path $testRoot 'scripts')
$null = New-Item -ItemType Directory -Path (Join-Path $testRoot 'site/scripts')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'publish-clip-shot-website.ps1') -Destination (Join-Path $testRoot 'scripts')
[IO.File]::WriteAllText((Join-Path $testRoot 'CMakeLists.txt'), 'set(CLIP_SHOT_VERSION "1.1.8")')
$fixture = @'
[CmdletBinding(SupportsShouldProcess)]
param($Version, $ServerHost, $ServerUser, $ServerPort, $IdentityFile, $KnownHostsFile, $RemoteWebRoot, $PublicBaseUrl)
if ($PSCmdlet.ShouldProcess($ServerHost, 'fixture deployment')) {
    $global:ClipWebsiteTestSettings = $PSBoundParameters
}
'@
[IO.File]::WriteAllText((Join-Path $testRoot 'site/scripts/publish-release.ps1'), $fixture)
$global:ClipWebsiteTestSettings = $null
try {
    $workflow = Join-Path $testRoot 'scripts/publish-clip-shot-website.ps1'
    $site = Join-Path $testRoot 'site'
    & $workflow -WebsiteDirectory $site
    Require ($global:ClipWebsiteTestSettings.Version -ceq '1.1.8') 'Use the source release version.'
    Require ($global:ClipWebsiteTestSettings.ServerHost -ceq '120.79.232.67') 'Use the official website host.'
    Require ($global:ClipWebsiteTestSettings.RemoteWebRoot -ceq '/var/www/html') 'Use the website directory.'
    $global:ClipWebsiteTestSettings = $null
    $failed = $false
    try { & $workflow -WebsiteDirectory $site -Version '1.1.9' } catch { $failed = $true }
    Require ($failed -and $null -eq $global:ClipWebsiteTestSettings) 'Reject a version different from the app release.'
    & $workflow -WebsiteDirectory $site -WhatIf
    Require ($null -eq $global:ClipWebsiteTestSettings) 'WhatIf must reach the nested workflow without deployment.'
    & $workflow -WebsiteDirectory $site -ServerPort 2222 -RemoteWebRoot '/srv/clipshot'
    Require ($global:ClipWebsiteTestSettings.ServerPort -eq 2222 -and
        $global:ClipWebsiteTestSettings.RemoteWebRoot -ceq '/srv/clipshot') 'Forward server overrides.'
    Write-Output 'Clip website integration tests passed.'
} finally {
    $resolvedTestRoot = [IO.Path]::GetFullPath($testRoot)
    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $resolvedTestRoot.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing to remove a fixture outside the temporary directory.'
    }
    Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    Remove-Variable ClipWebsiteTestSettings -Scope Global
}
