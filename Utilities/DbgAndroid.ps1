param(
	[Parameter(Mandatory=$true)]
	[string]$packageName,
	
	[Parameter(Mandatory=$true)]
	[string[]]$originalSoDir,
	
	[string]$arch,
	[string]$detectArch,
	
	[string]$activityName = "android.app.NativeActivity",
	[string]$debugTemp = "debugtemp",
	[string]$apk,
	[switch]$PrepareOnly,
	[switch]$PrintCmds,
	[switch]$MessageBoxOnError,
	[switch]$StartLogcat
)

$ErrorActionPreference = "Stop"

if($MessageBoxOnError)
{
	[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms") | Out-Null
}

$debugPort=5039

function RaiseError
{
	param(
		[string]$msg
	)
	if($MessageBoxOnError)
	{
		[System.Windows.Forms.MessageBox]::Show($msg)
	}
	else
	{
		Write-Host $msg -foreground red
	}
	exit 1
}

if((-not $arch) -and (-not $detectArch))
{
	RaiseError "Either the -arch or the -detectArch switch must be given"
}

if($detectArch)
{
	$detectArch = $detectArch.ToLower()
	if(($detectArch -match "x64") -or ($detectArch -match "x86_64"))
	{
		$arch = "x86_64"
	}
	elseif($detectArch -match "x86")
	{
		$arch = "x86"
	}
	elseif($detectArch -match "arm64")
	{
		$arch = "arm64"
	}
	elseif($detectArch -match "arm")
	{
		$arch = "arm"
	}
	else
	{
		RaiseError "Failed to detect architecture from '$detectArch'"
	}
	Write-Host "Detected architecture" $arch
}

#Ensure the debugtemp directory exists
if(-not (Test-Path $debugTemp))
{
	New-Item -Path $debugTemp -ItemType "directory" -Force
}
$debugTemp = Resolve-Path $debugTemp

$adb = "$env:ANDROID_HOME\platform-tools\adb.exe"
if(-not (Test-Path $adb))
{
	RaiseError "Failed to find adb executable in $adb. Please ensure that the ANDROID_HOME environment variable is correctly set"
}

$jdb = "$env:JAVA_HOME\bin\jdb.exe"
if(-not (Test-Path $jdb))
{
	RaiseError "Failed to find jdb executable in $jdb. Please ensure taht the JAVA_HOME environment variable is correctly set."
}
$jdb = Resolve-Path $jdb

function Adb-Shell
{
	param(
		[string]$cmd,
		[string]$failureMsg = ("Error executing adb shell command: {0}" -f $cmd)
	)
	if($PrintCmds)
	{
		Write-Host "Executing: adb shell" $cmd
	}
	try 
	{
		$($result = (& $adb shell $cmd *>&1)) | Out-Null
	}
	catch
	{
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("{0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f $failureMsg, ($result | Out-String), $callstack)
	}
	if ($lastexitcode -ne 0)
	{
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("{0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f $failureMsg, ($result | Out-String), $callstack)		
	}
	return $result
}

function Adb-Cmd
{
	param(
		[Parameter(
		Mandatory=$True,
		ValueFromRemainingArguments=$true,
		Position = 0
		)][string[]]
		$cmds
	)
	if($PrintCmds)
	{
		Write-Host "Executing: adb" $cmds
	}
	
	$result = ""
	$errorAction = $ErrorActionPreference
	try
	{
		$ErrorActionPreference = "Continue"
		$result = (& $adb $cmds *>&1)
	}
	finally
	{
		$ErrorActionPreference = $errorAction
	}
	if ($lastexitcode -ne 0)
	{
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("Failed to execute adb {0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f ($cmds -join " "), ($result -join "`n"), $callstack)	
	}
	return $result -join "`n"
}

# find the gdb server executable
$gdbServerLocalPath = "$env:ANDROID_NDK_HOME/prebuilt/android-${arch}/gdbserver/gdbserver"
if(-not (Test-Path $gdbServerLocalPath))
{
	RaiseError "Could not find gdbserver in expected location: $gdbServerLocalPath. Please ensure that the ANDROID_NDK_HOME environment variable is correctly set."
}
$gdbServerLocalPath = Resolve-Path $gdbServerLocalPath

#find the gdb executable
$gdbPath = "$env:ANDROID_NDK_HOME/prebuilt/windows-x86_64/bin/gdb.exe"
if(-not (Test-Path $gdbPath))
{
	RaiseError "Could not find gdb in expected location: $gdbPath. Please ensure that the ANDROID_NDK_HOME environment variable is correctly set."
}

if($apk)
{
	if(-not (Test-Path $apk))
	{
		RaiseError "Failed to find .apk in specified location: $apk."
	}
	Adb-Cmd install $apk
}

# get the devices API level
[int]$deviceApiLevel = [convert]::ToInt32($(Adb-Shell 'getprop "ro.build.version.sdk"'), 10)
Write-Host "Device API Level is $deviceApiLevel"

#Get the application data directory
$appDir = $(Adb-Shell "run-as $packageName /system/bin/sh -c pwd 2>/dev/null").Trim()
Write-Host "Application directory is $appDir"
$debugSocketFile = "$appDir/debug_socket"

# Check if device is rooted or run-as works correctly
$userIsRoot = (Adb-Shell "id") -match "root"
if(-not $userIsRoot)
{
	$runAsBroken = (Adb-Shell "run-as $packageName /system/bin/sh -c pwd") -match "unknown"
	if($runAsBroken)
	{
		RaiseError "ERROR: your device has a broken run-as and is not rooted. Can not debug."
	}
}

# Check if the gdbserver is still running from a previous session
$gdbServerInfo = (Adb-Shell "ps") -match "gdbserver"
if($gdbServerInfo)
{
	$gdbServerPid = ($gdbServerInfo -replace "\s+"," " -split " ")[1]
	try
	{
		Adb-Shell "run-as $packageName kill $gdbServerPid"
	}
	catch
	{
		RaiseError "A gdbserver is still running and failed to kill. Please manually ensure that there is no gdbserver running."
	}
}

# Tell the java application to wait for an java debugger
Adb-Shell "am set-debug-app -w $packageName"

# Start the app
Adb-Shell "am start $packageName/$activityName"

# Get the app PID
$appInfo = (Adb-Shell "ps") -match $packageName
$appPid = ($appInfo -replace "\s+"," " -split " ")[1]
while(-not $appPid)
{
	Write-Host "Waiting for app to start..."
	Start-Sleep 1
	$appInfo = (Adb-Shell "ps") -match $packageName
	$appPid = ($appInfo -replace "\s+"," " -split " ")[1]
}
Write-Host "App PID is" $appPid

if($StartLogcat)
{
	Start-Process -FilePath "$env:comspec" -ArgumentList "/C adb logcat --pid=$appPid"
}

# Forward the java debugger port
Adb-Cmd forward tcp:12345 jdwp:$appPid

# Forward the gdb port
Adb-Cmd forward tcp:$debugPort localfilesystem:$debugSocketFile

# Copy required files from device
$processExecutable = ""
$libraryPath = "/system/lib"
if($arch -match "64")
{
	$processExecutable = Join-Path $debugTemp "app_process64"
	Adb-Cmd pull "/system/bin/app_process64" $processExecutable
	$libraryPath = "/system/lib64"
	Adb-Cmd pull "/system/bin/linker64" "$debugTemp/linker64"
}
else
{
	if((Adb-Shell "ls /system/bin/app*" | Out-String) -match "app_process32")
	{
		$processExecutable = Join-Path $debugTemp "app_process32"
		Adb-Cmd pull "/system/bin/app_process32" $processExecutable
	}
	else
	{
		$processExecutable = Join-Path $debugTemp "app_process"
		Adb-Cmd pull "/system/bin/app_process" $processExecutable
	}
	Adb-Cmd pull "/system/bin/linker" "$debugTemp/linker"
}

Adb-Cmd pull "$libraryPath/libc.so" "$debugTemp/libc.so"

# Copy the gdb server to the device
$gdbServerName = "gdbserver-$arch"
$gdbServerRemotePath = "/data/local/tmp"
$appGdbserverPath = "$appDir"

$gdbServerRemoteTestPath = $gdbServerRemotePath
if($deviceApiLevel -ge 23)
{
	$gdbServerRemoteTestPath = $appGdbserverPath
}

# Test if the gdbserver executable already exists in the required location
$appFiles = Adb-Shell "run-as $packageName ls $gdbServerRemoteTestPath"

if(($appFiles | Out-String) -match $gdbServerName)
{
	if($PrintCmds)
	{
		Write-Host "gdbserver already present on device"
	}
	$gdbServerRemotePath = "$gdbServerRemoteTestPath/$gdbServerName"
}
else
{
	Write-Host "Copying gdb server from $gdbServerLocalPath to $gdbServerRemotePath/$gdbServerName"
	Adb-Cmd push $gdbServerLocalPath "$gdbServerRemotePath/$gdbServerName"

	# Move gdbserver to application directory if we can't execute from /data/local/tmp directly
	if($deviceApiLevel -ge 23)
	{
		if($deviceApiLevel -ge 24)
		{
			Adb-Shell "run-as $packageName /system/bin/chmod a+x $appDir"
		}
		Adb-Shell "run-as $packageName sh -c 'cat $gdbServerRemotePath/$gdbServerName > $appGdbserverPath/$gdbServerName'"
		Adb-Shell "run-as $packageName chmod 700 $appGdbserverPath/$gdbServerName"
		$gdbServerRemotePath = "$appGdbserverPath/$gdbServerName"
	}
}

# Start gdbserver
Start-Process -FilePath "$env:comspec" -ArgumentList "/C `"$adb shell run-as $packageName $gdbServerRemotePath --once +$debugSocketFile --attach $appPid`"" -WindowStyle Hidden

# Generate gdb config
$gdbConfig = "set solib-search-path $debugTemp;" + ($originalSoDir -join ";") + "`n"
$gdbConfig += "set print pretty on`n"
$gdbConfig += "file $processExecutable`n"
$gdbConfig += "set sysroot $debugTemp`n"

if(-not $PrepareOnly)
{
	$gdbConfig += "target remote :$debugPort`n"
}

$gdbConfig = $gdbConfig -replace '\\','\\'

$gdbConfig | Out-File $debugTemp/gdb.setup -Encoding ASCII -NoNewline


if($PrepareOnly)
{
	Start-Process -FilePath "$env:comspec" -ArgumentList "/C `"`"$jdb`" -connect com.sun.jdi.SocketAttach:port=12345,hostname=localhost`"" -WindowStyle Hidden
	
	Start-Sleep 1
}
else
{

	# Let the app start: 
	$jdbJob = Start-Job -Scriptblock {
		param(
			$jdb
		)
		Start-Sleep -Seconds 3
		Start-Process -FilePath "$env:comspec" -ArgumentList "/C `"`"$jdb`" -connect com.sun.jdi.SocketAttach:port=12345,hostname=localhost`" && pause"
	} -ArgumentList $jdb

	# Launch gdb
	& $gdbPath -x $debugTemp/gdb.setup
	
	# Recieve results from  jdb launch
	Receive-Job $jdbJob
}


