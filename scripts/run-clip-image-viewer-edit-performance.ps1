[CmdletBinding()]
param([Parameter(ValueFromRemainingArguments = $true)][object[]]$Arguments)
$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "../clip_image_viewer/scripts/run-edit-performance-matrix.ps1") @Arguments
exit $LASTEXITCODE
