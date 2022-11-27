. "$PSScriptRoot\common-functions.ps1"

$appPath = Find-EditorProcessor
"Using $appPath"

# Transform all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    "Transforming project $projectDir"

    & $appPath -project $projectDir -transform PC | Out-Null
}