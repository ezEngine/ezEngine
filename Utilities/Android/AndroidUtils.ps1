## ADB/JDB paths are resolved lazily on first use to avoid module-load failures
## when ANDROID_HOME or JAVA_HOME are not set.
## Use Get-Adb and Get-Jdb to retrieve the resolved paths.

$ErrorActionPreference = "Stop"
if ($MessageBoxOnError -and $IsWindows) {
	[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms") | Out-Null
}

function Get-LatestSdkTools {
	$cmdTools = "$Env:ANDROID_HOME/cmdline-tools"
	if (-not (Test-Path $cmdTools)) {
		RaiseError "Failed to find cmdline-tools folder in $cmdTools. Please ensure that the ANDROID_HOME environment variable is correctly set"
	}

	$folders = Get-ChildItem -Path $cmdTools -Directory

	$highestFloat = [float]::NegativeInfinity
	$highestFloatFolder = $null

	foreach ($folder in $folders) {
		if ($folder.Name -match "latest") {
			$highestFloatFolder = $folder
			break;
		}
		# Attempt to convert the folder name to a float
		$floatValue = 0.0
		if ([float]::TryParse($folder.Name, [ref]$floatValue)) {
			# Check if the converted float value is higher than the current highest
			if ($floatValue -gt $highestFloat) {
				$highestFloat = $floatValue
				$highestFloatFolder = $folder
			}
		}
	}
	if ($null -eq $highestFloatFolder) {
		RaiseError "Failed to find a valid cmdline-tools folder under $cmdTools. Please ensure that the ANDROID_HOME environment variable is correctly set"
	}

	return $highestFloatFolder.FullName
}

function Get-SdkManager {
	$sdkTools = Get-LatestSdkTools
	$sdkManager = Join-Path -Path $sdkTools -ChildPath "bin/sdkmanager"
	if ($IsWindows) {
		$sdkManager = "$sdkManager.bat"
	}
	if (-not (Test-Path $sdkManager)) {
		RaiseError "Failed to locate sdkmanager in $sdkManager. Please ensure that the ANDROID_HOME environment variable is correctly set"
	}
	return $sdkManager
}

function RaiseError {
	param(
		[string]$msg
	)

	if ($ErrorActionPreference -eq "Stop") {
		if ($MessageBoxOnError -and $IsWindows) {
			[System.Windows.Forms.MessageBox]::Show($msg)
		}
		else {
			Write-Host $msg -foreground red
		}
		exit 1
	}
	else {
		throw $msg
	}
}

function Get-Adb {
	# Returns the resolved adb path, resolving once and caching the result.
	if (-not $script:__adbPath) {
		$path = "$env:ANDROID_HOME/platform-tools/adb"
		if ($IsWindows) {
			$path = "$path.exe"
		}
		if (-not (Test-Path $path)) {
			RaiseError "Failed to find adb executable in $path. Please ensure that the ANDROID_HOME environment variable is correctly set"
		}
		$script:__adbPath = (Resolve-Path $path).Path
		# Also expose as $script:adb for any legacy consumers that expect the variable
		$script:adb = $script:__adbPath
	}
	return $script:__adbPath
}

function Get-Jdb {
	# Returns the resolved jdb path, resolving once and caching the result.
	if (-not $script:__jdbPath) {
		$path = "$env:JAVA_HOME/bin/jdb"
		if ($IsWindows) {
			$path = "$path.exe"
		}
		if (-not (Test-Path $path)) {
			RaiseError "Failed to find jdb executable in $path. Please ensure that the JAVA_HOME environment variable is correctly set."
		}
		$script:__jdbPath = (Resolve-Path $path).Path
		# Also expose as $script:jdb for any legacy consumers that expect the variable
		$script:jdb = $script:__jdbPath
	}
	return $script:__jdbPath
}

function Adb-Shell {
	param(
		[string]$cmd,
		[string]$deviceAdb,
		[string]$failureMsg = ("Error executing adb shell command: {0}" -f $cmd)
	)
	if ($PrintCmds) {
		Write-Host "Executing: adb shell" $cmd
	}
	try {
		if ($deviceAdb) {
			$($result = (& (Get-Adb) -s $deviceAdb shell $cmd *>&1)) | Out-Null
		}
		else {
			$($result = (& (Get-Adb) shell $cmd *>&1)) | Out-Null
		}

	}
	catch {
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("{0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f $failureMsg, ($result | Out-String), $callstack)
	}
	if ($lastexitcode -ne 0) {
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("{0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f $failureMsg, ($result | Out-String), $callstack)		
	}
	return $result
}

function Adb-Cmd {
	param(
		[Parameter(
			Mandatory = $True,
			ValueFromRemainingArguments = $true,
			Position = 0
		)][string[]]
		$cmds
	)
	if ($PrintCmds) {
		Write-Host "Executing: adb" $cmds
	}
	
	$result = ""
	$errorAction = $ErrorActionPreference
	try {
		$ErrorActionPreference = "Continue"
		$result = (& (Get-Adb) $cmds *>&1)
	}
	finally {
		$ErrorActionPreference = $errorAction
	}
	if ($lastexitcode -ne 0) {
		$callstack = Get-PSCallStack
		$callstack = $callstack[1..$callstack.Length] | % { $res = "" } { $res += $_.toString() + "`n" } { $res }
		RaiseError ("Failed to execute adb {0}`nOutput: {1}`n`nCallstack:`n{2}`n" -f ($cmds -join " "), ($result -join "`n"), $callstack)	
	}
	return $result -join "`n"
}

function Adb-CopyDirectory {
	param(
		[string]$deviceAdb,
		[string]$packageName,
		[string]$deviceFolder,
		[string]$outputFolder)
	
	[System.IO.Directory]::CreateDirectory($outputFolder) | Out-Null
	$result = Adb-Shell -deviceAdb $deviceAdb "run-as $packageName ls `"$deviceFolder`" -1 -p"

	ForEach ($line in $($result -split "`r`n")) {
		if ($line -match '/$') {
			# Folder need to recurse
			$folderName = $line.Trim("/")
			$outputFolderChild = Join-Path -Path $outputFolder -ChildPath $folderName
			$deviceFolderChild = "$deviceFolder/$folderName"
                
			Adb-CopyDirectory -deviceAdb $deviceAdb -packageName $packageName -outputFolder $outputFolderChild -deviceFolder $deviceFolderChild
		}
		else {
			# Files need to download
			$outputFileChild = Join-Path -Path $outputFolder -ChildPath $line
			$deviceFileChild = "$deviceFolder/$line"
			if ($IsWindows) {
				Start-Process -PassThru -WindowStyle Hidden -FilePath (Get-Adb) -ArgumentList "-s $deviceAdb exec-out run-as $packageName cat `"$deviceFileChild`"" -RedirectStandardOutput $outputFileChild | Out-Null
			}
			else {
				Start-Process -PassThru -FilePath (Get-Adb) -ArgumentList "-s $deviceAdb exec-out run-as $packageName cat `"$deviceFileChild`"" -RedirectStandardOutput $outputFileChild | Out-Null
			}	
		}
	}
}

function Invoke-WithRetry {
	param(
		[Parameter(Mandatory = $true)]
		[ScriptBlock]$ScriptBlock,
		[int]$MaxRetryCount = 3
	)

	$retryCount = 0
	$sleepInterval = 1
	do {
		try {
			& $ScriptBlock
			break
		}
		catch {
			Start-Sleep -Seconds $sleepInterval
			$sleepInterval = $sleepInterval * 2
			if ($retryCount -ge $MaxRetryCount) {
				RaiseError("Error: $_")
			}
			else {
				$retryCount++
				Write-Host "Error: $_`nRetrying ($retryCount/$MaxRetryCount)..."
			}
		}
	} while ($true)
}

# Functions to install sdkmanager packages. Extracted from InstallAndroidDependencies.ps1
function Install-SdkBuildDependencies {
	param(
		[Parameter(Mandatory = $true)][string]$SdkManager
	)

	& $SdkManager "cmdline-tools;latest"
	# Check if sdkmanager created a duplicate (latest-2) and replace latest with it
	$cmdlineLatest2 = Join-Path -Path $SdkManager -ChildPath "latest-2"
	if (Test-Path $cmdlineLatest2) {
		Write-Host "Detected latest-2 folder. Replacing latest with latest-2..."
		Remove-Item -LiteralPath $cmdlineLatest -Recurse -Force -ErrorAction SilentlyContinue
		Move-Item -Path $cmdlineLatest2 -Destination $cmdlineLatest -Force
	}
	& $SdkManager "build-tools;34.0.0" "ndk;26.1.10909125" "platform-tools" "platforms;android-29"
}

function Install-SdkEmulatorPackages {
	param(
		[Parameter(Mandatory = $true)][string]$SdkManager
	)

	& $SdkManager "emulator" "system-images;android-29;google_apis;x86_64"
	if ($IsWindows) {
		& $SdkManager "extras;google;Android_Emulator_Hypervisor_Driver"
	}
}

