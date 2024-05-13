param(
    [Parameter(Mandatory=$true)]
    [string]$deviceAdb,
    [Parameter(Mandatory=$true)]
    [string]$packageName,
    [Parameter(Mandatory=$true)]
    [string]$activityName,
    [Parameter(Mandatory=$true)]
    [string]$outputFolder,
    [string]$apk,
    [switch]$MessageBoxOnError
)

# Import Android utils
. "$PSScriptRoot/AndroidUtils.ps1"

# Install app
if ($apk) {
    Write-Host "Installing $apk..."
    $installFunction = {
        Adb-Cmd -ErrorAction Continue -s $deviceAdb install -r -t $apk
        # Starting the app right after install usually fails
        Start-Sleep -Seconds 1
    }
    Invoke-WithRetry -ScriptBlock $installFunction -MaxRetryCount 5
}

Write-Host "Clearing LogCat..."
Adb-Cmd -s $deviceAdb logcat --clear

Write-Host "Starting $packageName/$activityName..."
$startFunction = {
    Adb-Cmd -ErrorAction Continue -s $deviceAdb shell am start -n $packageName/$activityName
}
Invoke-WithRetry -ScriptBlock $startFunction -MaxRetryCount 5

# Define the path to the output log file
[System.IO.Directory]::CreateDirectory("$outputFolder") | Out-Null
$outputFilePath = Join-Path -Path "$outputFolder" -ChildPath "$packageName.log"

Write-Host "Starting LogCat..."
$pinfo = New-Object System.Diagnostics.ProcessStartInfo
$pinfo.FileName = "$adb"
$pinfo.RedirectStandardError = $false
$pinfo.RedirectStandardOutput = $true
$pinfo.UseShellExecute = $false
$pinfo.Arguments = "-s $deviceAdb logcat ezEngine:D *:S"

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $pinfo
$process.Start() | Out-Null

$startTime = Get-Date
$maxWaitTime = 300
$outputContent = ""
$testSuccess = $false;
$testFinished = $false;
$checkActivityTime = Get-Date
$processRunning = $true

# Parse stdout and wait for activity exit 
while ((Get-Date) -lt ($startTime.AddSeconds($maxWaitTime)) -and !$process.HasExited) {

    if ($null -eq $stdoutTask) {
        $stdoutTask = $process.StandardOutput.ReadLineAsync()
    }

    # Check if the activity is still running every 10 sec
    if ($processRunning -and (Get-Date) -gt ($checkActivityTime.AddSeconds(10))) {
        $checkActivityTime = Get-Date
        $stillRunning = & $adb -s $deviceAdb shell "dumpsys window windows | grep -E '$packageName/$activityName'"
        if ($stillRunning -eq "" -or $null -eq $stillRunning) {
            $processRunning = $false
        }
    }
    # If the activity ends, wait another 5 sec for logcat messages before killing it
    if (!$processRunning -and (Get-Date) -gt ($checkActivityTime.AddSeconds(5))) {
        Write-Host "Test activity exited unexpectedly"
        $process.Kill()
    }

    if ($stdoutTask.Wait(100)) {
        $line = $stdoutTask.Result
        if ( $null -ne $line) {
            # Log to console
            Write-Host "$line"

            if ($line -match "All tests passed.") {
                $testSuccess = $true
            }
            elseif ($line -match "Test framework exited with return code") {
                $testFinished = $true
                $process.Kill()
            }

            # Will write to file later
            $outputContent += "$line`n"
        }
        $stdoutTask = $process.StandardOutput.ReadLineAsync()
    }
}

if (-not $testFinished) {
    Write-Host "Test did not run to completion within $maxWaitTime seconds or logcat crashed!"
    $process.Kill()
}
else {
    if ($testSuccess) {
        Write-Host "All tests passed!"
    }
    else {
        Write-Host "Tests failed!"
    }
}

# Write logcat output to file
$outputContent | Out-File -Encoding "UTF8" $outputFilePath

# Download artifacts from device
Adb-CopyDirectory -deviceAdb $deviceAdb -packageName $packageName -outputFolder $outputFolder -deviceFolder "/data/data/$packageName/files"

# Sleep a bit before clearing the log
Start-Sleep -Seconds 1
Adb-Cmd -s $deviceAdb logcat --clear
Start-Sleep -Seconds 1

if ($testSuccess) {
    exit 0
} else {
    exit 1
}
