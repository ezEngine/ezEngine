param(
	[string]$packageName,	
	[string]$activityName = "android.app.NativeActivity",
	[string]$debugTemp = "debugtemp",
	[string]$apk,
	[switch]$PrintCmds,
	[switch]$MessageBoxOnError,
	[switch]$StartLogcat
)

#Requires -Version 7.0

# Import Android utils
. "$PSScriptRoot/AndroidUtils.ps1"

$ErrorActionPreference = "Stop"

# Find arch
$arch = Adb-Shell "getprop ro.product.cpu.abi"
if (-not $arch) {
	RaiseError "Failed to determine architecture of target device"
}

#Ensure the debugtemp directory exists
if (-not (Test-Path $debugTemp)) {
	New-Item -Path $debugTemp -ItemType "directory" -Force
}
$debugTemp = Resolve-Path $debugTemp

# Copy lldb-server to device if not already present
$TEMP_DIR = "/data/local/tmp"
$tempFiles = Adb-Shell "ls $TEMP_DIR"
$filesPresent = (($tempFiles | Out-String) -match "lldb-server" -and ($tempFiles | Out-String) -match "start_lldb_server.sh")
if (-not $filesPresent) {
	# find the lldb-server executable
	$androidStudioPath = "$env:ANDROID_STUDIO"
	if (-not (Test-Path $androidStudioPath)) {
		RaiseError "Failed to find Android Studio in $androidStudioPath. Please ensure that the ANDROID_STUDIO environment variable is correctly set. Alternatively, run the Android Studio debugger once to install lldb-server"
	}

	$lldbLocalPath = "$androidStudioPath/plugins/android-ndk/resources/lldb/android"
	$lldbServerLocalPath = "$lldbLocalPath/$arch/lldb-server"
	$lldbStartLocalPath = "$lldbLocalPath/start_lldb_server.sh"

	if (-not (Test-Path $lldbServerLocalPath)) {
		RaiseError "Could not find lldb-server in expected location: $lldbServerLocalPath. Please ensure that the ANDROID_NDK_HOME environment variable is correctly set."
	}
	$lldbServerLocalPath = Resolve-Path $lldbServerLocalPath

	if (-not (Test-Path $lldbStartLocalPath)) {
		RaiseError "Could not find start_lldb_server.sh in expected location: $lldbStartLocalPath. Please ensure that the ANDROID_NDK_HOME environment variable is correctly set."
	}
	$lldbStartLocalPath = Resolve-Path $lldbStartLocalPath

	# Copy the lldb server to the device tmp folder
	Adb-Cmd push "$lldbServerLocalPath" $TEMP_DIR
	Adb-Cmd push "$lldbStartLocalPath" $TEMP_DIR
}

if ($apk) {
	if (-not (Test-Path $apk)) {
		RaiseError "Failed to find .apk in specified location: $apk."
	}
	Adb-Cmd install $apk
}

# Get the devices API level
[int]$deviceApiLevel = [convert]::ToInt32($(Adb-Shell 'getprop "ro.build.version.sdk"'), 10)
Write-Host "Device API Level is $deviceApiLevel"

# Get the application data directory
$appDir = $(Adb-Shell "run-as $packageName /system/bin/sh -c pwd 2>/dev/null").Trim()
Write-Host "Application directory is $appDir"

# Check if device is rooted or run-as works correctly
$userIsRoot = (Adb-Shell "id") -match "root"
if (-not $userIsRoot) {
	$runAsBroken = (Adb-Shell "run-as $packageName /system/bin/sh -c pwd") -match "unknown"
	if ($runAsBroken) {
		RaiseError "ERROR: your device has a broken run-as and is not rooted. Can not debug."
	}
}

# Check if the lldb-server is still running from a previous session
$lldbServerInfo = & $adb shell pidof lldb-server
if ($null -ne $lldbServerInfo) {
	$lldbServerPids = $lldbServerInfo.Split(" ")
	if ($lldbServerPids) {
		foreach ($lldbServerPid in $lldbServerPids) {
			# Ignore results as there might be multiple instances running from other packages.
			& $adb shell run-as $packageName kill $lldbServerPid
		}	
	}
}

# Tell the java application to wait for an java debugger
Adb-Shell "am set-debug-app -w $packageName"

# Start the app
Adb-Shell "am start $packageName/$activityName"

# Get the app PID
$appInfo = (Adb-Shell "ps") -match $packageName
$appPid = ($appInfo -replace "\s+", " " -split " ")[1]
while (-not $appPid) {
	Write-Host "Waiting for app to start..."
	Start-Sleep 1
	$appInfo = (Adb-Shell "ps") -match $packageName
	$appPid = ($appInfo -replace "\s+", " " -split " ")[1]
}
Write-Host "App PID is" $appPid

if ($StartLogcat) {
	Start-Process -FilePath "$env:comspec" -ArgumentList "/C adb logcat --pid=$appPid"
}

# Forward the java debugger port
Adb-Cmd forward tcp:12345 jdwp:$appPid

# Copy required files from device
$processExecutable = ""
$libraryPath = "/system/lib"
if ($arch -match "64") {
	$processExecutable = Join-Path $debugTemp "app_process64"
	Adb-Cmd pull "/system/bin/app_process64" $processExecutable
	$libraryPath = "/system/lib64"
	Adb-Cmd pull "/system/bin/linker64" "$debugTemp/linker64"
}
else {
	if ((Adb-Shell "ls /system/bin/app*" | Out-String) -match "app_process32") {
		$processExecutable = Join-Path $debugTemp "app_process32"
		Adb-Cmd pull "/system/bin/app_process32" $processExecutable
	}
	else {
		$processExecutable = Join-Path $debugTemp "app_process"
		Adb-Cmd pull "/system/bin/app_process" $processExecutable
	}
	Adb-Cmd pull "/system/bin/linker" "$debugTemp/linker"
}

Adb-Cmd pull "$libraryPath/libc.so" "$debugTemp/libc.so"

# Copy from tmp into app folder
$lldbServerRemoteTestPath = $TEMP_DIR
if ($deviceApiLevel -ge 23) {
	$lldbServerRemoteTestPath = "$appDir"
}

$LLDB_DIR = "/data/data/${packageName}/lldb"
$LLDB_DIR = "${lldbServerRemoteTestPath}/lldb"

Adb-Shell "run-as ${packageName} mkdir -p ${LLDB_DIR}/bin"
Adb-Shell "cat ${TEMP_DIR}/lldb-server | run-as ${packageName} sh -c 'cat > ${LLDB_DIR}/bin/lldb-server && chmod 700 ${LLDB_DIR}/bin/lldb-server'"
Adb-Shell "cat ${TEMP_DIR}/start_lldb_server.sh | run-as ${packageName} sh -c 'cat > ${LLDB_DIR}/bin/start_lldb_server.sh && chmod 700 ${LLDB_DIR}/bin/start_lldb_server.sh'"

# Start lldbserver
& $adb shell "run-as ${packageName} ${LLDB_DIR}/bin/start_lldb_server.sh ${LLDB_DIR} unix-abstract /${packageName} debug.socket `"lldb process:gdb-remote packets`"" &

# Generate lldb config
$lldbConfig = "platform select remote-android`n"
$lldbConfig += "platform connect unix-abstract-connect:///${packageName}/debug.socket`n"
$lldbConfig += "attach ${appPid}`n"
$lldbConfig += "process handle -p true -n true -s false SIGSEGV`n"
$lldbConfig += "process handle -p true -n true -s false SIGBUS`n"
$lldbConfig | Out-File $debugTemp/lldb.setup -Encoding ASCII -NoNewline

Write-Host "Done"
Start-Sleep -Seconds 1

