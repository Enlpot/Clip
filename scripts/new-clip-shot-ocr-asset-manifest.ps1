#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OcrRuntimeArchive,
    [ValidateSet('x64', 'arm64')][string]$Architecture = 'arm64',
    [Parameter(Mandatory)][string]$Output
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'clip-shot-ocr-release-runtime.ps1')
New-ClipOcrAssetManifest -ArchivePath $OcrRuntimeArchive -Architecture $Architecture -OutputPath $Output
