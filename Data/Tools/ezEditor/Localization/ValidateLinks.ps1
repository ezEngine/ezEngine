$Files = (Get-ChildItem . -Include *.txt -Recurse).fullname

foreach ($File in $Files) {
    Write-Host "Validating URLs in file '$File'"

    foreach ($line in Get-Content $File) {
        if ($line -match "https:.*.html") {
            $url = $Matches[0]

            $error.Clear()
            $time = Measure-Command { Invoke-WebRequest -Uri $url } 2>$null

            if ($error.Count -eq 0) {

            }
            else {
                Write-Host "  Invalid URL: '$url'"
            }            
        }
    }    
}

Write-Host "Done."