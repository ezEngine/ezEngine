# Reduces texture sizes in a folder by converting DDS/TGA files to JPG/PNG using ezTexConv,
# and optionally optimizes the resulting JPG and PNG files.
#
# Usage:
#   ./reduce-texture-sizes.ps1 "C:\path\to\folder" [-Convert] [-OptimizeJpeg] [-OptimizePng]
#
# Options:
#   -TargetFolder   Folder to process recursively. Defaults to current directory.
#   -Convert        Run ezTexConv in reduce mode to convert DDS/TGA to JPG/PNG (deletes source files).
#   -OptimizeJpeg   Run optimize-jpeg.ps1 on all JPG/JPEG files afterwards.
#   -OptimizePng    Run optimize-png.ps1 on all PNG files afterwards.

param(
    [string]$TargetFolder = ".",
    [switch]$Convert,
    [switch]$OptimizeJpeg,
    [switch]$OptimizePng
)

. "$PSScriptRoot\common-functions.ps1"

if (-not (Test-Path $TargetFolder -PathType Container)) {
    Write-Error "Target folder '$TargetFolder' does not exist."
    exit 1
}

$TargetFolder = [System.IO.Path]::GetFullPath($TargetFolder)

# ---- Convert DDS/TGA -> JPG/PNG via ezTexConv ----

if ($Convert) {
    $TexConv = Find-EzExe "ezTexConv.exe"

    if (-not $TexConv) {
        Write-Error "ezTexConv.exe not found. Please build the project first."
        exit 1
    }

    Write-Host "Running ezTexConv in reduce mode on '$TargetFolder' ..." -ForegroundColor Cyan
    & $TexConv -mode Reduce -in "$TargetFolder\*" -deleteSource true
    if ($LASTEXITCODE -ne 0) {
        Write-Error "ezTexConv failed with exit code $LASTEXITCODE."
        exit 1
    }
}

# ---- Optimize JPEG files ----

if ($OptimizeJpeg) {
    Write-Host "Optimizing JPEG files in '$TargetFolder' ..." -ForegroundColor Cyan
    & "$PSScriptRoot\optimize-jpeg.ps1" $TargetFolder
    if ($LASTEXITCODE -ne 0) {
        Write-Error "optimize-jpeg.ps1 failed with exit code $LASTEXITCODE."
        exit 1
    }
}

# ---- Optimize PNG files ----

if ($OptimizePng) {
    Write-Host "Optimizing PNG files in '$TargetFolder' ..." -ForegroundColor Cyan
    & "$PSScriptRoot\optimize-png.ps1" $TargetFolder
    if ($LASTEXITCODE -ne 0) {
        Write-Error "optimize-png.ps1 failed with exit code $LASTEXITCODE."
        exit 1
    }
}
