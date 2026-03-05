# To compress PNG files as much as possible using optipng:
# Run this script with the target folder as argument:
# $ ./optimize-png.ps1 "C:\path\to\folder"
# If no folder is given, the current directory is used.

param(
    [string]$TargetFolder = "."
)

$OptiPng = "$PSScriptRoot\..\..\Data\Tools\Precompiled\optipng\optipng.exe"
$OptiPng = [System.IO.Path]::GetFullPath($OptiPng)

if (-not (Test-Path $OptiPng -PathType Leaf)) {
    Write-Error "optipng not found at '$OptiPng'."
    exit 1
}

if (-not (Test-Path $TargetFolder -PathType Container)) {
    Write-Error "Target folder '$TargetFolder' does not exist."
    exit 1
}

Get-ChildItem -Path $TargetFolder -Filter *.png -Recurse | Where-Object { $_.FullName -notmatch '\\AssetCache\\' } | ForEach-Object {
    & $OptiPng -o7 -quiet "$($_.FullName)"
    Write-Host "Optimized in-place: $($_.FullName)" -ForegroundColor Green
}
