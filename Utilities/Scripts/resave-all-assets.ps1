# Find EditorProcessor.exe

$appPath = ""

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019Debug64\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019Dev64\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}    

$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs2019Shipping64\EditorProcessor.exe"
if (Test-Path $fileToCheck -PathType leaf)
{
    $appPath = $fileToCheck
}    

"Using $appPath"

# Transform all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    "Re-saving assets in project $projectDir"

    & $appPath -project $projectDir -resave | Out-Null
}