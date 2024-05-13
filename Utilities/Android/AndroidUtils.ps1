
# Find ADB and JDB
$adb = "$env:ANDROID_HOME/platform-tools/adb"
if ($IsWindows) {
	$adb = "$adb.exe"
}
if (-not (Test-Path $adb)) {
	RaiseError "Failed to find adb executable in $adb. Please ensure that the ANDROID_HOME environment variable is correctly set"
}
$adb = Resolve-Path $adb

$jdb = "$env:JAVA_HOME/bin/jdb"
if ($IsWindows) {
	$jdb = "$jdb.exe"
}
if (-not (Test-Path $jdb)) {
	RaiseError "Failed to find jdb executable in $jdb. Please ensure that the JAVA_HOME environment variable is correctly set."
}
$jdb = Resolve-Path $jdb

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
			$($result = (& $adb -s $deviceAdb shell $cmd *>&1)) | Out-Null
		}
		else {
			$($result = (& $adb shell $cmd *>&1)) | Out-Null
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
		$result = (& $adb $cmds *>&1)
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
			Start-Process -PassThru -WindowStyle Hidden -FilePath "$adb" -ArgumentList "-s $deviceAdb exec-out run-as $packageName cat `"$deviceFileChild`"" -RedirectStandardOutput $outputFileChild | Out-Null
		}
	}
}

function Invoke-WithRetry {
    param(
        [Parameter(Mandatory=$true)]
        [ScriptBlock]$ScriptBlock,
        [int]$MaxRetryCount = 3
    )

    $retryCount = 0

    do {
        try {
            & $ScriptBlock
            break
        }
        catch {
			Start-Sleep -Seconds 1
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