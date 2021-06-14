# Find EditorProcessor.exe

$appPath = ""

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019Release64\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019RelDeb64\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019Debug\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}

"Using $appPath"

# Transform all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    "Transforming project $projectDir"

    & $appPath -project $projectDir -transform PC | Out-Null
}