# Shared helpers for the release test scripts.
#
# Every Test-*.ps1 dot-sources this file, records its checks through Add-TestResult
# and finishes with Save-TestResults, which writes "Results.<Group>.json" into the
# output directory. Run-ReleaseTests.ps1 merges those files into a summary.

Set-StrictMode -Version Latest

$script:TestResults = @()
$script:TestGroup = "Unknown"
$script:TestOutputDir = ""

function global:Initialize-TestGroup
{
	param([string]$Group, [string]$OutputDir)

	$script:TestGroup = $Group
	$script:TestResults = @()
	$script:TestOutputDir = $OutputDir

	if (-not (Test-Path $OutputDir))
	{
		New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
	}

	Write-Host ""
	Write-Host "=== $Group ===" -ForegroundColor Cyan
}

function global:Add-TestResult
{
	param(
		[string]$Name,
		[ValidateSet("PASS", "FAIL", "SKIP")][string]$Status,
		[double]$DurationSeconds = 0,
		[string]$Message = "",
		[string]$Artifact = "")

	$script:TestResults += [PSCustomObject]@{
		Group    = $script:TestGroup
		Name     = $Name
		Status   = $Status
		Duration = [math]::Round($DurationSeconds, 1)
		Message  = $Message
		Artifact = $Artifact
	}

	$color = switch ($Status) { "PASS" { "Green" } "FAIL" { "Red" } default { "Yellow" } }
	$line = "[{0}] {1}" -f $Status, $Name
	if ($DurationSeconds -gt 0) { $line += " ({0:N1}s)" -f $DurationSeconds }
	if ($Message) { $line += " - $Message" }
	Write-Host $line -ForegroundColor $color
}

# Runs a script block, turns an exception or a $false return value into a FAIL result.
# The block may return a string, which is used as the result message.
function global:Invoke-TestCheck
{
	param(
		[string]$Name,
		[scriptblock]$Check,
		[string]$Artifact = "",
		# reported instead of the time the check itself took, for checks that only evaluate
		# the outcome of an already finished process
		[double]$DurationSeconds = -1)

	$sw = [System.Diagnostics.Stopwatch]::StartNew()

	try
	{
		$msg = & $Check
		$sw.Stop()
		$duration = if ($DurationSeconds -ge 0) { $DurationSeconds } else { $sw.Elapsed.TotalSeconds }

		if ($msg -is [bool])
		{
			if ($msg) { Add-TestResult -Name $Name -Status "PASS" -DurationSeconds $duration -Artifact $Artifact }
			else { Add-TestResult -Name $Name -Status "FAIL" -DurationSeconds $duration -Artifact $Artifact }
		}
		else
		{
			$text = [string]($msg | Select-Object -Last 1)

			# A block that neither threw nor returned anything has not actually checked what it claims to
			# check - passing it silently would hide a broken check as a green result.
			if ([string]::IsNullOrWhiteSpace($text))
			{
				Add-TestResult -Name $Name -Status "FAIL" -DurationSeconds $duration -Artifact $Artifact -Message "The check returned no result."
			}
			else
			{
				Add-TestResult -Name $Name -Status "PASS" -DurationSeconds $duration -Message $text -Artifact $Artifact
			}
		}
	}
	catch
	{
		$sw.Stop()
		$duration = if ($DurationSeconds -ge 0) { $DurationSeconds } else { $sw.Elapsed.TotalSeconds }
		Add-TestResult -Name $Name -Status "FAIL" -DurationSeconds $duration -Message $_.Exception.Message -Artifact $Artifact
	}
}

function global:Save-TestResults
{
	$file = Join-Path $script:TestOutputDir ("Results.{0}.json" -f $script:TestGroup)
	# an empty array must still produce '[]', hence the array subexpression
	ConvertTo-Json -InputObject @($script:TestResults) -Depth 4 | Set-Content -Path $file -Encoding UTF8

	$failed = @($script:TestResults | Where-Object { $_.Status -eq "FAIL" }).Count
	Write-Host ("{0}: {1} checks, {2} failed" -f $script:TestGroup, $script:TestResults.Count, $failed)
	return $failed
}

# --------------------------------------------------------------------------------------
# Package layout
# --------------------------------------------------------------------------------------

# Returns the binary folder of a packaged or built SDK, e.g. <SdkDir>/Output/Bin/WinVs2026Dev64.
function global:Get-EzBinDir
{
	param(
		[Parameter(Mandatory = $true)][string]$SdkDir,
		# explicit binary folder, for testing a build workspace whose output is not below the SDK
		[string]$BinDir = "")

	if ($BinDir)
	{
		if (-not (Test-Path (Join-Path $BinDir "ezPlayer.exe")))
		{
			throw "'$BinDir' contains no ezPlayer.exe."
		}

		return (Resolve-Path $BinDir).Path
	}

	$binRoot = Join-Path $SdkDir "Output/Bin"

	if (-not (Test-Path $binRoot))
	{
		throw "'$SdkDir' does not look like an ezEngine SDK, '$binRoot' is missing."
	}

	# prefer the newest compiler and the fastest configuration that is actually present
	foreach ($config in @("Shipping", "Dev", "Debug"))
	{
		foreach ($vs in @("2026", "2022"))
		{
			$dir = Join-Path $binRoot ("WinVs{0}{1}64" -f $vs, $config)
			if (Test-Path (Join-Path $dir "ezPlayer.exe"))
			{
				return (Resolve-Path $dir).Path
			}
		}
	}

	throw "No binary folder with ezPlayer.exe found below '$binRoot'."
}

function global:Get-EzExe
{
	param([Parameter(Mandatory = $true)][string]$BinDir, [Parameter(Mandatory = $true)][string]$ExeName)

	$path = Join-Path $BinDir $ExeName

	if (-not (Test-Path $path -PathType Leaf))
	{
		throw "'$ExeName' does not exist in '$BinDir'."
	}

	return $path
}

# --------------------------------------------------------------------------------------
# Process helpers
# --------------------------------------------------------------------------------------

# Builds a Windows command line from separate arguments, quoting entries that contain a space so
# they arrive as one argument (e.g. the path of the 'Testing Chambers' sample).
# ProcessStartInfo.ArgumentList is not used for this: it requires .NET Core, but the release test
# runs under Windows PowerShell 5.1 / .NET Framework, where that property does not exist.
function global:ConvertTo-EzArgumentString
{
	param([string[]]$Arguments)

	($Arguments | ForEach-Object {
		if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
	}) -join ' '
}

# Kills a process and everything it spawned.
# Process.Kill(bool entireProcessTree) is not used: that overload only exists on .NET Core, while the
# release test runs under Windows PowerShell 5.1 / .NET Framework, where it throws MethodNotFound.
# Stop-Process does not walk the tree either, so taskkill does it.
function global:Stop-ProcessTreeById
{
	param([int]$Id)

	# taskkill reports 'process not found' on stderr for an already dead process, which is not an error here
	try { & taskkill.exe /PID $Id /T /F 2>&1 | Out-Null } catch { }
}

# Reads an already started ReadToEndAsync task, but never waits longer than TimeoutSeconds.
# The task only completes once every handle on the pipe is closed, and a surviving grandchild process
# inherits that handle - so blocking on .Result can hang for as long as that grandchild lives.
function global:Get-StreamTaskResult
{
	param([System.Threading.Tasks.Task[string]]$Task, [string]$StreamName, [int]$TimeoutSeconds = 10)

	try
	{
		if ($Task.Wait($TimeoutSeconds * 1000)) { return $Task.Result }
	}
	catch { }

	return "<$StreamName was not closed within ${TimeoutSeconds}s, output is incomplete>"
}

# Runs a process to completion and returns exit code, stdout, stderr and duration.
# TimeoutSeconds is the hard kill limit - the tested tools have their own -timeout options,
# this is only the safety net around them.
function global:Invoke-EzProcess
{
	param(
		[string]$Exe,
		[string[]]$Arguments,
		[string]$WorkingDirectory = "",
		[int]$TimeoutSeconds = 600)

	$psi = [System.Diagnostics.ProcessStartInfo]::new()
	$psi.FileName = $Exe
	$psi.RedirectStandardOutput = $true
	$psi.RedirectStandardError = $true
	$psi.UseShellExecute = $false
	$psi.CreateNoWindow = $true
	if ($WorkingDirectory) { $psi.WorkingDirectory = $WorkingDirectory }
	$psi.Arguments = ConvertTo-EzArgumentString -Arguments $Arguments

	$proc = [System.Diagnostics.Process]::new()
	$proc.StartInfo = $psi

	$sw = [System.Diagnostics.Stopwatch]::StartNew()

	try
	{
		$proc.Start() | Out-Null

		# read both streams asynchronously, otherwise a full pipe buffer deadlocks the child
		$stdOutTask = $proc.StandardOutput.ReadToEndAsync()
		$stdErrTask = $proc.StandardError.ReadToEndAsync()

		$timedOut = $false

		if (-not $proc.WaitForExit($TimeoutSeconds * 1000))
		{
			$timedOut = $true
			Stop-ProcessTreeById -Id $proc.Id
			$proc.WaitForExit(10000) | Out-Null
		}

		$sw.Stop()

		return [PSCustomObject]@{
			ExitCode = if ($timedOut) { -999 } else { $proc.ExitCode }
			StdOut   = Get-StreamTaskResult -Task $stdOutTask -StreamName "stdout"
			StdErr   = Get-StreamTaskResult -Task $stdErrTask -StreamName "stderr"
			Duration = $sw.Elapsed.TotalSeconds
			TimedOut = $timedOut
		}
	}
	finally
	{
		$sw.Stop()
		$proc.Dispose()
	}
}

function global:Start-EzProcessDetached
{
	param([string]$Exe, [string[]]$Arguments, [string]$WorkingDirectory = "")

	# Start-Process is not used here: it concatenates arguments without quoting, so an argument
	# containing a space (e.g. the path of the 'Testing Chambers' sample) arrives as two arguments.
	$psi = [System.Diagnostics.ProcessStartInfo]::new()
	$psi.FileName = $Exe
	$psi.UseShellExecute = $false
	# the applications write their whole log to stdout, which would otherwise end up in the console
	# of the test run; it is captured and can be written to a file with Save-DetachedProcessOutput
	$psi.RedirectStandardOutput = $true
	$psi.RedirectStandardError = $true
	if ($WorkingDirectory) { $psi.WorkingDirectory = $WorkingDirectory }
	$psi.Arguments = ConvertTo-EzArgumentString -Arguments $Arguments

	$proc = [System.Diagnostics.Process]::new()
	$proc.StartInfo = $psi
	$proc.Start() | Out-Null

	# the pipes have to be drained, a full buffer would block the child process
	Add-Member -InputObject $proc -NotePropertyName "EzStdOutTask" -NotePropertyValue $proc.StandardOutput.ReadToEndAsync()
	Add-Member -InputObject $proc -NotePropertyName "EzStdErrTask" -NotePropertyValue $proc.StandardError.ReadToEndAsync()

	return $proc
}

# Writes what a detached process has printed so far to a file. Only returns anything useful once
# the process has exited, the read tasks complete with it.
function global:Save-DetachedProcessOutput
{
	param([System.Diagnostics.Process]$Process, [string]$LogFile)

	if ($null -eq $Process -or -not $Process.HasExited) { return }

	try
	{
		$out = Get-StreamTaskResult -Task $Process.EzStdOutTask -StreamName "stdout"
		$err = Get-StreamTaskResult -Task $Process.EzStdErrTask -StreamName "stderr"
		Set-Content -Path $LogFile -Value ($out + "`n" + $err) -Encoding UTF8
	}
	catch { }
}

function global:Stop-EzProcessTree
{
	param([System.Diagnostics.Process]$Process)

	if ($null -eq $Process) { return }

	try
	{
		if (-not $Process.HasExited)
		{
			# the editor spawns an engine process, killing only the parent would leave it behind
			Stop-ProcessTreeById -Id $Process.Id
			$Process.WaitForExit(10000) | Out-Null
		}
	}
	catch { }
}

# Waits until $Condition returns $true. Polls instead of sleeping for a fixed duration,
# so a fast startup doesn't cost the full wait.
function global:Wait-ForCondition
{
	param([scriptblock]$Condition, [int]$TimeoutSeconds = 60, [int]$PollMilliseconds = 250)

	$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)

	while ([DateTime]::UtcNow -lt $deadline)
	{
		if (& $Condition) { return $true }
		Start-Sleep -Milliseconds $PollMilliseconds
	}

	return $false
}

function global:Get-LeftoverEzProcesses
{
	param([string]$BinDir)

	$names = @("ezEditor", "ezEditorEngineProcess", "ezEditorProcessor", "ezPlayer")

	# a prefix comparison, not -like: a '[' anywhere in the path would be a wildcard character there
	$prefix = (Join-Path $BinDir "")

	return @(Get-Process -Name $names -ErrorAction SilentlyContinue | Where-Object {
		try { $_.Path -and $_.Path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase) } catch { $false }
	})
}

# --------------------------------------------------------------------------------------
# Log file inspection
# --------------------------------------------------------------------------------------

# Returns the error/serious warning lines of a player log file. ezPlayer -failonerror already
# fails the run for these, this is only for reporting what went wrong.
function global:Get-LogErrors
{
	param([string]$LogFile, [int]$MaxLines = 5)

	if (-not (Test-Path $LogFile)) { return @() }

	# the lines are prefixed with a timestamp and indented by their log block nesting
	return @(Select-String -Path $LogFile -Pattern '(Error|SeriousWarning): ' -ErrorAction SilentlyContinue |
		Select-Object -First $MaxLines -ExpandProperty Line | ForEach-Object { $_.Trim() })
}

# --------------------------------------------------------------------------------------
# Image inspection
# --------------------------------------------------------------------------------------

Add-Type -AssemblyName PresentationCore -ErrorAction SilentlyContinue

# Decodes an image into a byte array of BGRA pixels.
function global:Get-ImagePixels
{
	param([string]$Path)

	$uri = [System.Uri]::new((Resolve-Path $Path).Path)
	$frame = [System.Windows.Media.Imaging.BitmapFrame]::Create($uri,
		[System.Windows.Media.Imaging.BitmapCreateOptions]::PreservePixelFormat,
		[System.Windows.Media.Imaging.BitmapCacheOption]::OnLoad)

	$converted = [System.Windows.Media.Imaging.FormatConvertedBitmap]::new($frame, [System.Windows.Media.PixelFormats]::Bgra32, $null, 0)

	$stride = $converted.PixelWidth * 4
	$pixels = [byte[]]::new($stride * $converted.PixelHeight)
	$converted.CopyPixels($pixels, $stride, 0)

	return [PSCustomObject]@{
		Width  = $converted.PixelWidth
		Height = $converted.PixelHeight
		Pixels = $pixels
	}
}

# Mean and standard deviation of the luminance of an image.
# A screenshot of a project that started but rendered nothing is a single flat color,
# i.e. a standard deviation of (nearly) zero.
function global:Get-ImageLuminanceStats
{
	param([string]$Path)

	$img = Get-ImagePixels -Path $Path
	$pixels = $img.Pixels
	$count = [int]($pixels.Length / 4)

	# sub-sampled, a full 1080p image would be slow to walk in PowerShell
	$step = [math]::Max(1, [int]($count / 20000))

	$sum = 0.0
	$sumSq = 0.0
	$samples = 0

	for ($i = 0; $i -lt $count; $i += $step)
	{
		$o = $i * 4
		$lum = 0.0722 * $pixels[$o] + 0.7152 * $pixels[$o + 1] + 0.2126 * $pixels[$o + 2]
		$sum += $lum
		$sumSq += $lum * $lum
		$samples++
	}

	$mean = $sum / $samples
	$variance = [math]::Max(0.0, ($sumSq / $samples) - ($mean * $mean))

	return [PSCustomObject]@{
		Width  = $img.Width
		Height = $img.Height
		Mean   = $mean
		StdDev = [math]::Sqrt($variance)
	}
}

# Mean square error between two images of identical size, on 0-255 luminance.
# Used for the optional reference image comparison, matching what the test framework does
# for its own image tests.
function global:Get-ImageMeanSquareError
{
	param([string]$PathA, [string]$PathB)

	$a = Get-ImagePixels -Path $PathA
	$b = Get-ImagePixels -Path $PathB

	if ($a.Width -ne $b.Width -or $a.Height -ne $b.Height)
	{
		throw "Image sizes differ: $($a.Width)x$($a.Height) vs $($b.Width)x$($b.Height)."
	}

	$count = [int]($a.Pixels.Length / 4)
	$step = [math]::Max(1, [int]($count / 50000))

	$sum = 0.0
	$samples = 0

	for ($i = 0; $i -lt $count; $i += $step)
	{
		$o = $i * 4
		for ($c = 0; $c -lt 3; $c++)
		{
			$d = [double]$a.Pixels[$o + $c] - [double]$b.Pixels[$o + $c]
			$sum += $d * $d
		}
		$samples += 3
	}

	return $sum / $samples
}

# --------------------------------------------------------------------------------------
# MCP client
# --------------------------------------------------------------------------------------

function global:Test-McpPortOpen
{
	param([int]$Port)

	$client = [System.Net.Sockets.TcpClient]::new()

	try
	{
		$task = $client.ConnectAsync("127.0.0.1", $Port)
		return $task.Wait(500) -and $client.Connected
	}
	catch { return $false }
	finally { $client.Dispose() }
}

$script:McpRequestId = 0

# Sends a JSON-RPC request to an MCP server and returns the parsed response.
# The server answers on the main thread of the host application, so calls that trigger real work
# (asset transform, C++ build, project export) need a large timeout.
function global:Invoke-McpRequest
{
	param(
		[int]$Port,
		[string]$Method,
		[hashtable]$Params = @{},
		[int]$TimeoutSeconds = 120)

	$script:McpRequestId++

	$body = @{
		jsonrpc = "2.0"
		id      = $script:McpRequestId
		method  = $Method
		params  = $Params
	} | ConvertTo-Json -Depth 10 -Compress

	# -Proxy $null: without it a machine with a configured system proxy sends even loopback requests
	# through it, which either stalls or answers with the proxy's error page
	$response = Invoke-RestMethod -Uri "http://127.0.0.1:$Port/mcp" -Method Post -Body $body `
		-ContentType "application/json" -TimeoutSec $TimeoutSeconds -Proxy $null

	if ($null -ne $response.PSObject.Properties["error"] -and $null -ne $response.error)
	{
		throw "MCP '$Method' failed: $($response.error | ConvertTo-Json -Depth 5 -Compress)"
	}

	return $response.result
}

function global:Invoke-McpTool
{
	param([int]$Port, [string]$Tool, [hashtable]$Arguments = @{}, [int]$TimeoutSeconds = 120)

	$result = Invoke-McpRequest -Port $Port -Method "tools/call" -TimeoutSeconds $TimeoutSeconds `
		-Params @{ name = $Tool; arguments = $Arguments }

	# a tool that fails reports it in-band, via isError, not as a JSON-RPC error
	if ($null -ne $result.PSObject.Properties["isError"] -and $result.isError)
	{
		throw "MCP tool '$Tool' reported an error: $(($result.content | ForEach-Object { $_.text }) -join ' ')"
	}

	return $result
}

# Most tools return their payload as a JSON document inside a text content block.
function global:Get-McpToolJson
{
	param([int]$Port, [string]$Tool, [hashtable]$Arguments = @{}, [int]$TimeoutSeconds = 120)

	$result = Invoke-McpTool -Port $Port -Tool $Tool -Arguments $Arguments -TimeoutSeconds $TimeoutSeconds
	$text = ($result.content | Where-Object { $_.type -eq "text" } | ForEach-Object { $_.text }) -join "`n"

	try { return $text | ConvertFrom-Json }
	catch { return $text }
}
