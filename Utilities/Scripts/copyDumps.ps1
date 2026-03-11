param (
    [string]$InputPath,
    [string]$OutputPath
)

# Ensure the output path exists
if (!(Test-Path -Path $OutputPath)) {
    New-Item -ItemType Directory -Path $OutputPath | Out-Null
}

# Get all .exe files from the input path
$exeFiles = Get-ChildItem -Path $InputPath -Filter *.exe -File

# Define the default crash dumps location
$crashDumpsPath = Join-Path -Path $env:LOCALAPPDATA -ChildPath "CrashDumps"

# Check if the crash dumps directory exists
if (!(Test-Path -Path $crashDumpsPath)) {
    Write-Host "Crash dumps directory does not exist: $crashDumpsPath"
    exit 0
}

# Loop through each .exe file and copy matching .dmp files
foreach ($exe in $exeFiles) {
    $exeName = $exe.BaseName
    $matchingDumps = Get-ChildItem -Path $crashDumpsPath -Filter "$exeName*.dmp" -File

    foreach ($dump in $matchingDumps) {
        $destination = Join-Path -Path $OutputPath -ChildPath $dump.Name
        Copy-Item -Path $dump.FullName -Destination $destination -Force
        Write-Host "Copied: $($dump.FullName) -> $destination"
    }
}

Write-Host "Dump file copy operation completed."