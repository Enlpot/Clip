[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'clip-shot-github-release.ps1')
function Require([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Must-Fail([scriptblock]$Action) {
    $failed = $false
    try { & $Action | Out-Null } catch { $failed = $true }
    Require $failed 'Expected operation to fail.'
}
$root = Join-Path ([IO.Path]::GetTempPath()) ("clip-github-tests-$([guid]::NewGuid().ToString('N'))")
$null = New-Item -ItemType Directory -Path $root
$global:ClipGitHubTestFailDownload = $false
$global:ClipGitHubTestRelease = $null
$global:ClipGitHubTestBytes = @{}
$global:ClipGitHubTestEvents = [Collections.Generic.List[string]]::new()
function global:gh {
    $arguments = @($args)
    $global:LASTEXITCODE = 0
    if ($arguments[0] -eq 'api') {
        if ($arguments[1] -like '*/commits/*') { return ('a' * 40) }
        # --slurp returns one array per API page.
        if ($global:ClipGitHubTestRelease) { return ConvertTo-Json -InputObject @(@($global:ClipGitHubTestRelease)) -Depth 10 -Compress }
        return '[[]]'
    }
    $command = $arguments[1]
    $global:ClipGitHubTestEvents.Add($command)
    switch ($command) {
        create {
            Require (-not $global:ClipGitHubTestRelease) 'Do not create a duplicate release.'
            $global:ClipGitHubTestRelease = @{ tag_name = $arguments[2]; draft = $true; prerelease = ($arguments -contains '--prerelease'); assets = @() }
            if ($arguments -contains '--notes-file') {
                $global:ClipGitHubTestRelease.body = [IO.File]::ReadAllText($arguments[[array]::IndexOf($arguments, '--notes-file') + 1])
            }
        }
        upload {
            $path = $arguments[3]
            $name = [IO.Path]::GetFileName($path)
            Require (-not $global:ClipGitHubTestBytes.ContainsKey($name)) 'Never overwrite an asset.'
            $global:ClipGitHubTestBytes[$name] = [IO.File]::ReadAllBytes($path)
            $global:ClipGitHubTestRelease.assets += @{ name = $name }
        }
        download {
            if ($global:ClipGitHubTestFailDownload) { throw 'Injected GitHub asset download failure.' }
            $name = $arguments[[array]::IndexOf($arguments, '--pattern') + 1]
            $directory = $arguments[[array]::IndexOf($arguments, '--dir') + 1]
            Require ($global:ClipGitHubTestBytes.ContainsKey($name)) "Asset missing: $name"
            [IO.File]::WriteAllBytes((Join-Path $directory $name), $global:ClipGitHubTestBytes[$name])
        }
        edit {
            Require ($global:ClipGitHubTestEvents.Contains('download')) 'Verify before publishing.'
            $global:ClipGitHubTestRelease.draft = $false
            $global:ClipGitHubTestLatest = $arguments -contains '--latest'
        }
        default { throw "Unexpected gh operation: $command" }
    }
}
try {
    $path = Join-Path $root 'latest-version.json'
    [IO.File]::WriteAllText($path, 'signed fixture')
    $assets = @{ 'latest-version.json' = $path }
    $verify = Join-Path $root 'verify'
    Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify
    Require (-not $global:ClipGitHubTestRelease.draft -and $global:ClipGitHubTestLatest) 'Stable release becomes public and latest.'
    Require (($global:ClipGitHubTestEvents -join ',') -eq 'create,upload,download,edit') 'Upload/verify/publish ordering.'
    $global:ClipGitHubTestEvents.Clear()
    Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify
    Require (($global:ClipGitHubTestEvents -join ',') -eq 'download,download') 'Identical retry makes no mutations.'
    $global:ClipGitHubTestEvents.Clear()
    Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify -VerifyOnly
    Require (($global:ClipGitHubTestEvents -join ',') -eq 'download') 'Verification never mutates releases.'
    [IO.File]::WriteAllText($path, 'different bytes')
    Must-Fail { Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify }
    Require (-not $global:ClipGitHubTestEvents.Contains('upload')) 'Conflicts rejected before upload.'
    $global:ClipGitHubTestRelease = $null
    $global:ClipGitHubTestBytes = @{}
    $global:ClipGitHubTestEvents.Clear()
    Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0-beta' $assets $verify
    Require ($global:ClipGitHubTestRelease.prerelease -and -not $global:ClipGitHubTestLatest) 'Beta never becomes latest.'
    $global:ClipGitHubTestRelease.prerelease = $false
    Must-Fail { Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0-beta' $assets $verify }

    $global:ClipGitHubTestRelease = $null
    $global:ClipGitHubTestBytes = @{}
    $global:ClipGitHubTestEvents.Clear()
    $global:ClipGitHubTestFailDownload = $true
    Must-Fail { Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify }
    Require ($global:ClipGitHubTestRelease.draft -and -not $global:ClipGitHubTestEvents.Contains('edit')) 'Failed verification leaves a draft.'
    $global:ClipGitHubTestFailDownload = $false
    Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $assets $verify
    $missing = @{ 'missing.zip' = $path }
    Must-Fail { Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' $missing $verify }

    # Execute the real orchestration with isolated files, real RSA signing, and mocked
    # packaging audit/network/CLI boundaries. No production settings or server are used.
    $fixture = Join-Path $root 'repository'
    $fixtureScripts = Join-Path $fixture 'scripts'
    $resources = Join-Path $fixture 'clip_shot/resources'
    $packaging = Join-Path $fixture 'clip_shot/packaging'
    $build = Join-Path $fixture 'build'
    foreach ($directory in @($fixtureScripts, $resources, $packaging, $build)) { $null = New-Item -ItemType Directory -Force -Path $directory }
    foreach ($name in @('publish-clip-shot-release.ps1','clip-shot-github-release.ps1','clip-shot-editions.ps1','clip-shot-native-validation.ps1')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $fixtureScripts
    }
    @'
function Get-ClipWindowsHostArchitecture { return 'x64' }
function Get-ClipWindowsTarget {
    param($Architecture)
    $preset = if ($Architecture -eq 'arm64') { 'clip-shot-msvc-arm64-release' } else { 'clip-shot-msvc-release' }
    return [pscustomobject]@{ ReleasePreset = $preset }
}
'@ | Set-Content -LiteralPath (Join-Path $fixtureScripts 'clip-build-environment.ps1')
    @'
param($Version, $WebsiteDirectory)
Require ($Version -ceq '2.0.0' -and $WebsiteDirectory -ceq 'fixture-site') 'Forward the release website identity.'
Require ($global:ClipGitHubTestEvents.Contains('edit')) 'Deploy website after verified GitHub publication.'
$global:ClipGitHubTestEvents.Add('website')
'@ | Set-Content -LiteralPath (Join-Path $fixtureScripts 'publish-clip-shot-website.ps1')
    Set-Content -LiteralPath (Join-Path $fixture 'CMakeLists.txt') -Value 'set(CLIP_SHOT_VERSION "2.0.0")'
    $global:ClipGitHubTestRsa = [Security.Cryptography.RSA]::Create(3072)
    $keyPath = Join-Path $root 'test-key.pem'
    [IO.File]::WriteAllText($keyPath, $global:ClipGitHubTestRsa.ExportRSAPrivateKeyPem())
    $public = $global:ClipGitHubTestRsa.ExportParameters($false)
    @{ keys = @(@{ id = 'test'; modulus = [Convert]::ToBase64String($public.Modulus); exponent = [Convert]::ToBase64String($public.Exponent) }) } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $resources 'update-trusted-keys.json')
    $global:ClipGitHubTestOcr = Join-Path $root 'ocr.zip'
    [IO.File]::WriteAllText($global:ClipGitHubTestOcr, 'ocr')
    @{ runtime = @{ version = '1'; archive = @{ url = 'https://example.invalid/ocr'; size = 3; sha256 = (Get-FileHash $global:ClipGitHubTestOcr).Hash.ToLowerInvariant() } } } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $packaging 'clip-shot-ocr-asset-manifest.json')
    Copy-Item -LiteralPath (Join-Path $packaging 'clip-shot-ocr-asset-manifest.json') -Destination (Join-Path $packaging 'clip-shot-ocr-asset-manifest-arm64.json')
    foreach ($architecture in @('x64', 'arm64')) {
    foreach ($prefix in @('clip-shot', 'clip-shot-mini')) {
    $variants = if ($prefix -eq 'clip-shot-mini') { @('online','portable') } else { @('online','offline','portable') }
    foreach ($variant in $variants) {
        $kinds = if ($variant -eq 'portable') { @('portable') } else { @('installer','update') }
        foreach ($kind in $kinds) {
            $suffix = if ($kind -eq 'installer') { '.exe' } elseif ($kind -eq 'update') { '-update.zip' } else { '.zip' }
            $base = "$prefix-2.0.0-windows-$architecture-$variant"
            $package = Join-Path $build "$base$suffix"
            [IO.File]::WriteAllText($package, 'package')
            $hash = (Get-FileHash $package).Hash.ToLowerInvariant()
            [IO.File]::WriteAllText("$package.sha256", "$hash  $base$suffix`n")
            $preset = if ($architecture -eq 'arm64') { 'clip-shot-msvc-arm64-release' } else { 'clip-shot-msvc-release' }
            $manifest = @{ PackageVersion = '2.0.0'; Variant = $variant; Preset = $preset; StaticCrt = $true; StaticQt = $true;
                Architecture = $architecture; Installer = @{ Sha256 = $hash; Bytes = 7 }; Archive = @{ Sha256 = $hash; Bytes = 7 }; InstallFiles = @();
                NativeValidationRequired = $false; NativeValidation = @{ Platform = "windows-$architecture"; HostPlatform = "windows-$architecture"; Passed = $true;
                    Artifacts = @(@{ Name = "$base$suffix"; Bytes = 7; Sha256 = $hash }) } }
            $manifestSuffix = if ($kind -eq 'update') { '-update.manifest.json' } else { '.manifest.json' }
            $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $build "$base$manifestSuffix")
        }
    }
    }
    }
    function global:Invoke-WebRequest { param($Uri, $OutFile, $TimeoutSec, $MaximumRedirection)
        Require ($Uri.AbsoluteUri -eq 'https://example.invalid/ocr') 'Unexpected public network access.'
        Copy-Item -LiteralPath $global:ClipGitHubTestOcr -Destination $OutFile
    }
    function global:git { $global:LASTEXITCODE = 0; return ('a' * 40) }
    function global:Start-Process { param($FilePath, $ArgumentList, $WindowStyle, [switch]$PassThru, $RedirectStandardError)
        Require ($FilePath.EndsWith('clip-shot-updater.exe') -or $FilePath.EndsWith('clip-shot-mini-updater.exe')) 'Only the local auditor can run.'
        Require (-not $global:ClipGitHubTestEvents.Contains('upload')) 'Audit must precede every upload.'
        Require ($ArgumentList -contains '--static-only' -and $ArgumentList -contains '--platform') 'Offline cross-architecture audit must keep executable probes separate from native proof.'
        $global:ClipGitHubTestEvents.Add('audit')
        $manifestPath = $ArgumentList[[array]::IndexOf($ArgumentList, '--manifest') + 1].Trim('"')
        $envelope = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        Require ($global:ClipGitHubTestRsa.VerifyData([Convert]::FromBase64String($envelope.payload), [Convert]::FromBase64String($envelope.signature),
            [Security.Cryptography.HashAlgorithmName]::SHA256, [Security.Cryptography.RSASignaturePadding]::Pss)) 'Envelope must be authenticated.'
        $process = [pscustomobject]@{ ExitCode = 0; HasExited = $true }
        $process | Add-Member -MemberType ScriptMethod -Name WaitForExit -Value { param($timeout) return $true }
        return $process
    }
    $global:ClipGitHubTestRelease = $null
    $global:ClipGitHubTestBytes = @{}
    $global:ClipGitHubTestEvents.Clear()
    & (Join-Path $fixtureScripts 'publish-clip-shot-release.ps1') -BuildDirectory $build -Arm64BuildDirectory $build -SigningKeyPath $keyPath -SkipBuild -SkipGitee
    Require ($global:ClipGitHubTestEvents[0] -eq 'audit' -and $global:ClipGitHubTestEvents[-1] -eq 'edit') 'GitHub-only publication audits before publishing.'
    Require ($global:ClipGitHubTestBytes.ContainsKey('latest-version.json') -and $global:ClipGitHubTestBytes.ContainsKey('latest-version-windows-arm64.json') -and $global:ClipGitHubTestBytes.Count -eq 52) 'Publish all sixteen packages, sidecars, audit manifests, and four signed feeds.'
    $miniEnvelope = [Text.Encoding]::UTF8.GetString($global:ClipGitHubTestBytes['latest-version-mini.json']) | ConvertFrom-Json
    $miniPayload = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($miniEnvelope.payload)) | ConvertFrom-Json
    Require ($miniPayload.product -ceq 'clip-shot-mini' -and $miniPayload.packages.Count -eq 3) 'Mini signs its three packages with a product identity.'
    $previousEnvelope = [Convert]::ToBase64String($global:ClipGitHubTestBytes['latest-version.json'])
    $global:ClipGitHubTestEvents.Clear()
    & (Join-Path $fixtureScripts 'publish-clip-shot-release.ps1') -BuildDirectory $build -Arm64BuildDirectory $build -SigningKeyPath $keyPath -SkipBuild -AuditOnly -DeployWebsite -WebsiteDirectory 'fixture-site'
    Require (-not $global:ClipGitHubTestEvents.Contains('upload') -and -not $global:ClipGitHubTestEvents.Contains('edit')) 'AuditOnly never mutates GitHub.'
    Require (-not $global:ClipGitHubTestEvents.Contains('website')) 'AuditOnly never deploys the website.'
    Require ([Convert]::ToBase64String($global:ClipGitHubTestBytes['latest-version.json']) -ceq $previousEnvelope) 'Retry preserves authenticated envelope.'
    # Exercise the default two-channel orchestration with isolated local files.
    $notesPath = Join-Path $root 'notes.md'
    [IO.File]::WriteAllText($notesPath, "Detailed release notes.`n")
    $global:ClipGitHubTestNotes = [IO.File]::ReadAllText($notesPath)
    function global:python {
        $arguments = @($args)
        $global:LASTEXITCODE = 0
        Require ($arguments[0].EndsWith('publish-clip-shot-gitee-release.py')) 'Use the direct local Gitee publisher.'
        if ($arguments -contains '--check-auth') {
            Require (-not $global:ClipGitHubTestEvents.Contains('audit')) 'Credentials must be checked before packaging.'
            $global:ClipGitHubTestEvents.Add('gitee-auth')
            return
        }
        $manifest = Get-Content -Raw -LiteralPath $arguments[[array]::IndexOf($arguments, '--manifest') + 1] | ConvertFrom-Json
        Require ($manifest.body -ceq $global:ClipGitHubTestNotes) 'Both destinations use identical local notes.'
        Require ($manifest.sourceCommit -ceq ('a' * 40) -and $manifest.assets.Count -eq 52 -and $manifest.nativeValidation.Count -eq 16) 'Preserve the complete audited source, native proof, and asset contract.'
        foreach ($asset in $manifest.assets) {
            Require ((Get-FileHash -LiteralPath $asset.path).Hash.ToLowerInvariant() -ceq $asset.sha256) 'Gitee files must match local audited bytes.'
        }
        if ($arguments -contains '--prepare-homebrew') {
            Require ($global:ClipGitHubTestEvents.Contains('audit') -and -not $global:ClipGitHubTestEvents.Contains('upload')) 'Prepare distribution assets after auditing and before uploads.'
            $global:ClipGitHubTestEvents.Add('gitee-prepare')
        } else {
            Require ($global:ClipGitHubTestEvents.Contains('edit')) 'Publish Gitee after GitHub has been verified and published.'
            Require ($global:ClipGitHubTestRelease.body -ceq $manifest.body) 'GitHub notes must equal local Gitee notes.'
            $global:ClipGitHubTestEvents.Add('gitee-publish')
        }
    }
    $global:ClipGitHubTestEvents.Clear()
    $global:ClipGitHubTestRelease = $null
    $global:ClipGitHubTestBytes = @{}
    & (Join-Path $fixtureScripts 'publish-clip-shot-release.ps1') -BuildDirectory $build -Arm64BuildDirectory $build -SigningKeyPath $keyPath -SkipBuild -ReleaseNotesPath $notesPath -DeployWebsite -WebsiteDirectory 'fixture-site'
    Require ($global:ClipGitHubTestEvents[0] -eq 'gitee-auth' -and $global:ClipGitHubTestEvents[-2] -eq 'gitee-publish' -and
        $global:ClipGitHubTestEvents[-1] -eq 'website') 'Website deployment follows verified publication to both release destinations.'
    Must-Fail { Publish-ClipGitHubRelease 'mg-chao/clip-apps' '2.0.0' @{ 'latest-version.json' = $path } $verify -NotesPath $keyPath }
    $global:ClipGitHubTestEvents.Clear()
    & (Join-Path $fixtureScripts 'publish-clip-shot-release.ps1') -WhatIf -DeployWebsite -WebsiteDirectory 'fixture-site'
    Require ($global:ClipGitHubTestEvents.Count -eq 0) 'WhatIf does not build, sign, or contact GitHub.'
    Must-Fail { & (Join-Path $fixtureScripts 'publish-clip-shot-release.ps1') -Operation Rollback }
    $global:ClipGitHubTestRsa.Dispose()
    Write-Output 'PASS: GitHub release publication, retries, conflicts, classification, local signing, audit ordering, and dry runs.'
} finally {
    foreach ($name in @('gh','git','python','Invoke-WebRequest','Start-Process')) { Remove-Item "Function:/$name" -ErrorAction SilentlyContinue }
    Remove-Variable -Scope Global -Name 'ClipGitHubTest*' -ErrorAction SilentlyContinue
    $resolved = [IO.Path]::GetFullPath($root)
    $temporary = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($temporary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing to remove fixture outside temp.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
