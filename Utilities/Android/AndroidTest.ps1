<#
.SYNOPSIS
    Deploys and runs an ezEngine test application on an Android device via ADB.

.DESCRIPTION
    Installs an APK (if provided), launches the specified activity, captures logcat output, and waits for the test framework to report results. Downloads any output artifacts from the device when finished.

    The script exits with code 0 if all tests pass, or 1 on failure/timeout.

.PARAMETER deviceAdb
    ADB device identifier (serial number or IP:port for wireless debugging).

.PARAMETER packageName
    Android package name of the test application (e.g. "com.ezengine.FoundationTest").

.PARAMETER activityName
    Fully qualified activity class name (default: "android.app.NativeActivity").

.PARAMETER outputFolder
    Local directory where logcat output and downloaded artifacts are stored.

.PARAMETER apk
    Path to the APK file to install before running. If omitted, the script assumes the app is already installed on the device.

.PARAMETER arguments
    Extra command-line arguments passed to the test application via Android intent extras. The test framework reads these from the "args" intent string extra.

.PARAMETER MessageBoxOnError
    (Unused) Reserved for CI pipelines that show a message box on failure.

.EXAMPLE
    # Run FoundationTest with default settings (install APK + run all tests):
    pwsh ./Utilities/Android/AndroidTest.ps1 `
        -deviceAdb 192.168.178.77:5555 `
        -packageName com.ezengine.FoundationTest `
        -activityName android.app.NativeActivity `
        -outputFolder ./Output `
        -apk "./Output/Bin/AndroidNinjaClangDebugArm64/FoundationTest.apk"

.EXAMPLE
    # Run only the "Tracing" test group:
    pwsh ./Utilities/Android/AndroidTest.ps1 `
        -deviceAdb 192.168.178.77:5555 `
        -packageName com.ezengine.FoundationTest `
        -activityName android.app.NativeActivity `
        -outputFolder ./Output `
        -apk "./Output/Bin/AndroidNinjaClangDebugArm64/FoundationTest.apk" `
        -arguments "-run -noGui -all -filter Tracing"

.EXAMPLE
    # Run without re-installing (app already on device):
    pwsh ./Utilities/Android/AndroidTest.ps1 `
        -deviceAdb 192.168.178.77:5555 `
        -packageName com.ezengine.FoundationTest `
        -activityName android.app.NativeActivity `
        -outputFolder ./Output
#>
param(
    [Parameter(Mandatory=$true)]
    [string]$deviceAdb,
    [Parameter(Mandatory=$true)]
    [string]$packageName,
    [Parameter(Mandatory=$false)]
    [string]$activityName = "android.app.NativeActivity",
    [Parameter(Mandatory=$true)]
    [string]$outputFolder,
    [string]$apk,
    [string]$arguments,
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
        Start-Sleep -Seconds 2
    }
    Invoke-WithRetry -ScriptBlock $installFunction -MaxRetryCount 6
}

Write-Host "Clearing LogCat..."
Adb-Cmd -s $deviceAdb logcat --clear

Write-Host "Starting $packageName/$activityName..."
if ($arguments) {
    Write-Host "With arguments: $arguments"
    $startFunction = {
        Adb-Cmd -ErrorAction Continue -s $deviceAdb shell "am start -n $packageName/$activityName --es args '$arguments'"
    }
} else {
    $startFunction = {
        Adb-Cmd -ErrorAction Continue -s $deviceAdb shell am start -n $packageName/$activityName
    }
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
$pinfo.Arguments = "-s $deviceAdb logcat -s ezEngine"

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $pinfo
$process.Start() | Out-Null

$startTime = Get-Date
$maxWaitTime = 300
$outputLines = [System.Collections.Generic.List[string]]::new()
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
            $outputLines.Add($line)
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
$outputLines -join "`n" | Out-File -Encoding "UTF8" $outputFilePath

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
