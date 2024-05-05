param(
    [switch]$installBuildDependencies = $false,
    [switch]$installEmulator = $false,
    [switch]$startEmulator = $false,
    [switch]$stopEmulator = $false,   
    [string]$devicePort = "5555"
)

# Import Android utils
. "$PSScriptRoot/AndroidUtils.ps1"

# Emulator device constants
$deviceAdb = "emulator-${devicePort}"
$deviceName = "Pixel7"
$deviceAbi = "google_apis/x86_64"
$devicePackage = "system-images;android-29;google_apis;x86_64"
$deviceType = "pixel_7"

if ($installBuildDependencies) {
    $sdkTools = Get-LatestSdkTools
    $sdkManager = Join-Path -Path $sdkTools -ChildPath "bin/sdkmanager"
    if ($IsWindows) {
        $sdkManager = "$sdkManager.bat"
    }
    if (-not (Test-Path $sdkManager)) {
        RaiseError "Failed to locate sdkmanager in $sdkManager. Please ensure that the ANDROID_HOME environment variable is correctly set"
    }

    # build dependencies
    & $sdkManager "build-tools;34.0.0"
    & $sdkManager "cmdline-tools;latest"
    & $sdkManager "ndk;26.1.10909125"
    & $sdkManager "platform-tools"
    & $sdkManager "platforms;android-29"
    # Emulator packages
    & $sdkManager "emulator"
    & $sdkManager "extras;google;Android_Emulator_Hypervisor_Driver"
    & $sdkManager "system-images;android-29;google_apis;x86_64"

    for ($i = 0; $i -lt 8; $i++) {
        "y`ny`ny`n" | & $sdkManager --licenses | Out-Null
    } 
}

if ($installEmulator) {
    $sdkTools = Get-LatestSdkTools
    $avdManager = Join-Path -Path $sdkTools -ChildPath "bin/avdmanager"
    if ($IsWindows) {
        $avdManager = "$avdManager.bat"
    }
    if (-not (Test-Path $avdManager)) {
        RaiseError "Failed to locate avdmanager in $avdManager. Please ensure that the ANDROID_HOME environment variable is correctly set"
    }

    # Create device
    Write-Host "Installing emulator avd with name '$deviceName', abi '$deviceAbi' package '$devicePackage' based on device '$deviceType' ..."
    & $avdManager create avd --force --name $deviceName --abi $deviceAbi --package $devicePackage --device $deviceType
}

# Start Emulator
if ($startEmulator) {
    $emulator = "$Env:ANDROID_HOME/emulator/emulator"
    if ($IsWindows) {
        $emulator = "$emulator.exe"
    }
    if (-not (Test-Path $emulator)) {
        RaiseError "Failed to locate emulator in $emulator. Please ensure that the ANDROID_HOME environment variable is correctly set"
    }

    # Create a clean state
    Adb-Cmd kill-server
    Adb-Cmd start-server

    Write-Host "Starting emulator for device name '$deviceName' on port '$devicePort' ..."
    # -gpu host crashes a lot on AMD and shows some barrier problems.
    $emulatorProcess = Start-Process -filePath "$emulator" -ArgumentList "-avd $deviceName -wipe-data -no-snapshot -no-audio -port $devicePort -gpu swiftshader_indirect"
   
    # Check if emulator is running
    $maxWaitTime = 120
    $checkInterval = 5
    $startTime = Get-Date
    $booted = $false;
    # Loop until the emulator is fully booted or timeout occurs
    while ((Get-Date) -lt ($startTime.AddSeconds($maxWaitTime))) {
        # Check if emulator is running
        $devicesOutput = & $adb devices
        if ($devicesOutput -match "$deviceAdb") {
            # Check boot completion status
            $bootCompleted = & $adb -s $deviceAdb shell getprop sys.boot_completed
            if ($bootCompleted -eq "1") {
                Write-Host "Emulator fully booted."
                $booted = $true
                break
            } 
        }
        Start-Sleep -Seconds $checkInterval
    }

    if ($booted -eq $false) {
        RaiseError "Failed to start emulator within $maxWaitTime seconds"
    }
}

# Stop Emulator
if ($stopEmulator) {
    & $adb -s $deviceAdb emu kill
    return 0
}