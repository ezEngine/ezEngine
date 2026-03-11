[CmdletBinding(SupportsShouldProcess = $True)]

param 
(
    [Parameter(Mandatory = $True)] [string] $Marker
)

Get-ChildItem * -Include $Marker -Recurse | ForEach-Object {
    $folder = $_.Directory.FullName

    If ($WhatIfPreference) {
        Write-Host "What if: Excluding folder '$folder'"
    }
    else {
        Write-Host "Excluding folder '$folder'"
    }

    Remove-Item -Recurse -Force $folder
}
