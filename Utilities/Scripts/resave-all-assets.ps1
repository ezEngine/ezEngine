. "$PSScriptRoot\common-functions.ps1"

$appPath = Find-EditorProcessor
"Using $appPath"

# Re-save all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    "Re-saving assets in project $projectDir"

    & $appPath -project $projectDir -resave | Out-Null
}