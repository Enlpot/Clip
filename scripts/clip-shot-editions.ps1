# Shared product identities. Original defaults preserve historical release backfills.
function Get-ClipShotEdition([ValidateSet('Full', 'Mini')][string]$Edition = 'Full',
    [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64') {
    $feedSuffix = if ($Architecture -eq 'arm64') { '-windows-arm64' } else { '' }
    if ($Edition -eq 'Mini') {
        return [pscustomobject]@{ Edition = 'Mini'; Product = 'clip-shot-mini'; Name = 'Clip Mini';
            Executable = 'clip_shot_mini'; Registry = 'ClipShotMini'; Scoop = 'clipshot-mini';
            Winget = 'mg-chao.clip-shot-mini'; Feed = "latest-version-mini$feedSuffix.json";
            Variants = @('online', 'portable'); Component = 'ClipShotMini'; Marker = '__mini_data_directory' }
    }
    return [pscustomobject]@{ Edition = 'Full'; Product = 'clip-shot'; Name = 'Clip';
        Executable = 'clip_shot'; Registry = 'ClipShot'; Scoop = 'clipshot';
        Winget = 'mg-chao.clip-shot'; Feed = "latest-version$feedSuffix.json";
        Variants = @('online', 'offline', 'portable'); Component = 'ClipShot'; Marker = '__data_directory' }
}

function Get-ClipShotReleaseArchitectures($Release, [string]$Version) {
    $architectures = @('x64')
    if (@($Release.assets | Where-Object { $_.name -match 'windows-arm64' }).Count) {
        $architectures += 'arm64'
        foreach ($architecture in $architectures) {
        foreach ($edition in @('Full', 'Mini')) {
            $product = Get-ClipShotEdition $edition $architecture
            $names = @($product.Feed)
            foreach ($variant in $product.Variants) {
                $base = "$($product.Product)-$Version-windows-$architecture-$variant"
                $names += if ($variant -eq 'portable') { "$base.zip" } else { @("$base.exe", "$base-update.zip") }
            }
            foreach ($name in $names) {
                if (@($Release.assets | Where-Object { $_.name -ceq $name }).Count -ne 1) {
                    throw "Windows release is missing or duplicates $name."
                }
            }
        }
        }
    }
    return $architectures
}

function Get-ClipShotReleaseEditions($Release, [string]$Version) {
    $mini = @($Release.assets | Where-Object { $_.name -like 'clip-shot-mini-*' -or $_.name -like 'latest-version-mini*.json' })
    if ($mini.Count -eq 0) { return @('Full') }
    # A partial paired release must not submit only one edition to a package manager.
    foreach ($edition in @('Full', 'Mini')) {
        $product = Get-ClipShotEdition $edition
        $names = @($product.Feed)
        foreach ($variant in $product.Variants) {
            if ($variant -eq 'portable') { $names += "$($product.Product)-$Version-windows-x64-portable.zip" }
            else { $names += @("$($product.Product)-$Version-windows-x64-$variant.exe", "$($product.Product)-$Version-windows-x64-$variant-update.zip") }
        }
        foreach ($name in $names) {
            if (@($Release.assets | Where-Object { $_.name -ceq $name }).Count -ne 1) {
                throw "Paired release is missing or duplicates $name."
            }
        }
    }
    return @('Full', 'Mini')
}
