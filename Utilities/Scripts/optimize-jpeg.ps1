# To compress JPEG files as much as possible:
# Install mozjpeg from https://github.com/imagemin/mozjpeg-bin
# $ npm install --global mozjpeg
# Then run this script with the target folder as argument:
# $ ./optimize-jpeg.ps1 "C:\path\to\folder"
# If no folder is given, the current directory is used.

param(
    [string]$TargetFolder = "."
)

if (-not (Get-Command npx -ErrorAction SilentlyContinue)) {
    Write-Error "npx not found. Please install Node.js from https://nodejs.org and then run: npm install --global mozjpeg"
    exit 1
}

if (-not (Test-Path $TargetFolder -PathType Container)) {
    Write-Error "Target folder '$TargetFolder' does not exist."
    exit 1
}

Get-ChildItem -Path $TargetFolder -Recurse -Include *.jpg, *.jpeg | Where-Object { $_.FullName -notmatch '\\AssetCache\\' } | ForEach-Object {
    $tempFile = "$($_.FullName).tmp"

    # Compress to temp file
    npx mozjpeg -quality 80 -outfile "$tempFile" "$($_.FullName)"

    # If successful, overwrite the original
    if (Test-Path $tempFile) {
        Move-Item -Path $tempFile -Destination $_.FullName -Force
        Write-Host "Optimized in-place: $($_.FullName)" -ForegroundColor Green
    }
}
