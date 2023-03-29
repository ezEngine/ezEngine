. "$PSScriptRoot\common-functions.ps1"

$appPath = Find-EditorProcessor
Write-Host "Using $appPath"

# Transform all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName
    
    Write-Host "Transforming: -project $projectDir -transform PC"

    & $appPath -project $projectDir -transform PC | Out-Null
}