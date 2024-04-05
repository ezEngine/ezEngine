cls

$files = Get-ChildItem -Path $PSScriptRoot -Filter *.png 

$TCE = "$PSScriptRoot\..\..\..\..\Output\Bin\WinVs2022Debug64\ezTexConv.exe"

foreach($file in $files)
{
    $nameOnly = (Get-Item $file.Name).Basename

    Write-Host "$PSScriptRoot\$nameOnly.png"
    Write-Host "$PSScriptRoot\$nameOnly.dds"

    & $TCE -out "$PSScriptRoot\$nameOnly.dds" -in "$PSScriptRoot\$nameOnly.png" -rgba in0 -dilate true -compression Medium -usage Color -mipmaps Kaiser
}

