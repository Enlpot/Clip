# Deterministic edition/payload contracts. No builds, network, or application launches.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'clip-shot-editions.ps1')
. (Join-Path $PSScriptRoot 'package-clip-shot-mini.ps1') -FunctionsOnly
. (Join-Path $PSScriptRoot 'clip-shot-scoop.ps1')
function Require($Value, [string]$Message) { if (-not $Value) { throw $Message } }
function Must-Fail([scriptblock]$Action) {
    try { & $Action } catch { return }
    throw 'Expected invalid edition payload to be rejected.'
}
$fixture = Join-Path ([IO.Path]::GetTempPath()) "clip-shot-mini-release-$([guid]::NewGuid().ToString('N'))"
try {
    foreach ($name in @('bin/clip_shot_mini.exe', 'bin/clip-shot-mini-updater.exe',
            'bin/clip-shot-mini-mcp.exe', 'bin/crashpad_handler.exe',
            'bin/assets/ocr/asset-manifest.json', 'bin/audios/camera_shutter.mp3',
            'bin/__mini_data_directory', 'share/clip-shot-mini/licenses/LICENSE')) {
        $path = Join-Path $fixture $name
        $null = New-Item -ItemType Directory -Force (Split-Path $path)
        [IO.File]::WriteAllText($path, 'fixture')
    }
    Assert-ClipShotMiniPayload $fixture
    $forbidden = Join-Path $fixture 'bin/assets/ocr/models/engine.onnx'
    $null = New-Item -ItemType Directory -Force (Split-Path $forbidden)
    [IO.File]::WriteAllText($forbidden, 'forbidden')
    Must-Fail { Assert-ClipShotMiniPayload $fixture }
    Remove-Item -LiteralPath $forbidden
    # The current OCR runtime archive and completion records are not PE files.
    # They must also be rejected when accidentally staged elsewhere inside bin.
    foreach ($name in @('bin/clip-ocr-runtime-1.0.9-windows-x64.zip',
            'bin/runtime-manifest.json', 'bin/.complete.json',
            'share/clip-shot/assets/qrcode/detect.prototxt',
            'share/clip-shot-mini/assets/ocr/models/engine.onnx',
            'clip-ocr-runtime.zip')) {
        $forbidden = Join-Path $fixture $name
        $null = New-Item -ItemType Directory -Force (Split-Path $forbidden)
        [IO.File]::WriteAllText($forbidden, 'forbidden OCR runtime payload')
        Must-Fail { Assert-ClipShotMiniPayload $fixture }
        Remove-Item -LiteralPath $forbidden
    }
    if ($IsWindows) {
        # Hidden files must not bypass the Windows payload inventory.
        $forbidden = Join-Path $fixture 'bin/hidden-ocr-model.onnx'
        [IO.File]::WriteAllText($forbidden, 'inert test fixture')
        [IO.File]::SetAttributes($forbidden, [IO.FileAttributes]::Hidden)
        try { Must-Fail { Assert-ClipShotMiniPayload $fixture } }
        finally { Remove-Item -LiteralPath $forbidden -Force }
    }
    $forbidden = Join-Path $fixture 'bin/assets/qrcode/detect.prototxt'
    $null = New-Item -ItemType Directory -Force (Split-Path $forbidden)
    [IO.File]::WriteAllText($forbidden, 'forbidden')
    Must-Fail { Assert-ClipShotMiniPayload $fixture }
    Remove-Item -LiteralPath $forbidden
    [IO.File]::WriteAllText((Join-Path $fixture 'bin/clip_shot.exe'), 'wrong edition')
    Must-Fail { Assert-ClipShotMiniPayload $fixture }
    Remove-Item -LiteralPath (Join-Path $fixture 'bin/clip_shot.exe')
    $manifest = Join-Path $fixture 'bin/assets/ocr/asset-manifest.json'
    $repoRoot = Split-Path -Parent $PSScriptRoot
    foreach ($architecture in @('x64', 'arm64')) {
        $sourceName = if ($architecture -eq 'arm64') { 'clip-shot-ocr-asset-manifest-arm64.json' } else { 'clip-shot-ocr-asset-manifest.json' }
        $trusted = Join-Path $repoRoot "clip_shot/packaging/$sourceName"
        Copy-Item -LiteralPath $trusted -Destination $manifest -Force
        Assert-ClipShotMiniPayload -Stage $fixture -Architecture $architecture -OcrAssetManifest $trusted
        $opposite = if ($architecture -eq 'arm64') { 'x64' } else { 'arm64' }
        Must-Fail { Assert-ClipShotMiniPayload -Stage $fixture -Architecture $opposite -OcrAssetManifest $trusted }
        $tampered = Get-Content -LiteralPath $trusted -Raw | ConvertFrom-Json
        $tampered.models[0].files[0].sha256 = '0' * 64
        $tampered | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifest -Encoding utf8NoBOM
        Must-Fail { Assert-ClipShotMiniPayload -Stage $fixture -Architecture $architecture -OcrAssetManifest $trusted }
    }
    $armTrusted = Join-Path $repoRoot 'clip_shot/packaging/clip-shot-ocr-asset-manifest-arm64.json'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'clip_shot/packaging/clip-shot-ocr-asset-manifest.json') -Destination $manifest -Force
    Must-Fail { Assert-ClipShotMiniPayload -Stage $fixture -Architecture arm64 -OcrAssetManifest $armTrusted }
    Write-Output 'PASS: Mini trusted descriptor target, opposite architecture, and tampered model pin gates.'
    $mini = Get-ClipShotEdition Mini
    Require ($mini.Variants.Count -eq 2 -and $mini.Marker -ceq '__mini_data_directory') 'Mini has only online and portable variants and its own data marker.'
    $manifest = New-ClipShotScoopManifestObject '1.2.3' 'https://fixture.invalid/mini.zip' ('0' * 64) Mini
    Require ($manifest.bin[0][0] -ceq 'bin\clip_shot_mini.exe' -and $manifest.bin[0][1] -ceq 'clipshot-mini') 'Mini Scoop executable/shim identity is independent.'
    Require (($manifest.checkver.script -join "`n") -match 'clip-shot-mini-\$version') 'Mini Scoop checks its own assets.'
    Require ($manifest.autoupdate.architecture.'64bit'.url -match '/clip-shot-mini-') 'Mini Scoop upgrades its own edition.'
    $zip = Join-Path $fixture 'portable.zip'
    $archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($pair in @(@('bin/clip_shot_mini.exe', 'fixture'), @('bin/__mini_data_directory', 'portable'))) {
            $writer = [IO.StreamWriter]::new($archive.CreateEntry($pair[0]).Open())
            try { $writer.Write($pair[1]) } finally { $writer.Dispose() }
        }
    } finally { $archive.Dispose() }
    Assert-ClipShotScoopArchive $zip Mini
    Must-Fail { Assert-ClipShotScoopArchive $zip Full }
    $release = [pscustomobject]@{ assets = @([pscustomobject]@{ name = 'original-asset' }) }
    Require (@(Get-ClipShotReleaseEditions $release '1.2.3').Count -eq 1) 'Historical Full-only backfills are supported.'
    $release.assets += [pscustomobject]@{ name = 'latest-version-mini.json' }
    Must-Fail { Get-ClipShotReleaseEditions $release '1.2.3' }
    $release.assets = @(foreach ($edition in @('Full', 'Mini')) {
        $product = Get-ClipShotEdition $edition
        $names = @($product.Feed)
        foreach ($variant in $product.Variants) {
            if ($variant -eq 'portable') { $names += "$($product.Product)-1.2.3-windows-x64-portable.zip" }
            else { $names += @("$($product.Product)-1.2.3-windows-x64-$variant.exe", "$($product.Product)-1.2.3-windows-x64-$variant-update.zip") }
        }
        foreach ($name in $names) {
            [pscustomobject]@{ name = $name }
        }
    })
    Require (@(Get-ClipShotReleaseEditions $release '1.2.3').Count -eq 2) 'Both package-manager editions pass the paired release gate.'
    Write-Output 'PASS: Mini payload exclusions, portable archive, Scoop identity, and paired release gates.'
} finally {
    $absolute = [IO.Path]::GetFullPath($fixture)
    $temporary = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $absolute.StartsWith($temporary, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup path.' }
    if (Test-Path -LiteralPath $absolute) { Remove-Item -LiteralPath $absolute -Recurse -Force }
}
