# Checks that every tool that ships in a package starts and does not crash.
#
# Console tools are run with -help, which prints their options and exits. GUI tools show a message
# box for -help instead, so those are started normally, held for a moment and then closed again -
# that still catches the typical package failure of a missing DLL.
#
# Example:
#   Test-Tools.ps1 -SdkDir "D:\ez-test\ezEngine.Release.26.9.0" -OutputDir "D:\ez-test\results"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$SdkDir,
	[Parameter(Mandatory = $true)][string]$OutputDir,
	# explicit binary folder, e.g. a build workspace output; derived from -SdkDir when empty
	[string]$BinDir = "",
	# how long a GUI tool has to stay alive to count as 'started'
	[int]$AliveSeconds = 5
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. "$PSScriptRoot\ReleaseTestCommon.ps1"

$SdkDir = (Resolve-Path $SdkDir).Path
$binDir = Get-EzBinDir -SdkDir $SdkDir -BinDir $BinDir

Initialize-TestGroup -Group "Tools" -OutputDir $OutputDir

# Console applications: -help writes to stdout and exits.
# ezEditorProcessor is a GUI application as far as the linker is concerned, but it is headless and
# shows its help in a message box, so it is not in this list.
$consoleTools = @(
	"ezTexConv.exe",
	"ezShaderCompiler.exe",
	"ezArchiveTool.exe",
	"ezMiniDumpTool.exe",
	"ezFileserve.exe"
)

# GUI applications: started and closed again, they cannot report anything on stdout.
# The standalone sample applications (TextureSample, ShaderExplorer, ...) are not listed here even
# though they would fit the same pattern - they are samples, not tools, and Test-Samples.ps1 starts
# them for its -IncludeStandalone group. Having them in both lists only ran the identical check twice.
$guiTools = @(
	"ezEditor.exe",
	"ezPlayer.exe",
	"ezInspector.exe"
)

# --------------------------------------------------------------------------------------

foreach ($tool in $consoleTools)
{
	$exe = Join-Path $binDir $tool

	if (-not (Test-Path $exe))
	{
		Add-TestResult -Name "$tool -help" -Status "FAIL" -Message "Not present in the package."
		continue
	}

	$result = Invoke-EzProcess -Exe $exe -Arguments @("-help") -TimeoutSeconds 60

	Invoke-TestCheck -Name "$tool -help" -DurationSeconds $result.Duration -Check {
		if ($result.TimedOut) { throw "Did not exit - is it waiting for input?" }

		# a crash surfaces as an NTSTATUS exit code, e.g. 0xC0000005 for an access violation.
		# Ordinary non-zero codes are fine here: several tools use -1 to signal 'help was requested'
		# and others report 'nothing to do' that way.
		if ($result.ExitCode -le -1000)
		{
			throw "Crashed with exit code $($result.ExitCode) (0x$('{0:X8}' -f $result.ExitCode))."
		}

		if (($result.StdOut + $result.StdErr).Trim().Length -eq 0)
		{
			throw "Produced no output at all."
		}

		return "exit code $($result.ExitCode)"
	}
}

foreach ($tool in $guiTools)
{
	$exe = Join-Path $binDir $tool

	if (-not (Test-Path $exe))
	{
		Add-TestResult -Name "$tool starts" -Status "FAIL" -Message "Not present in the package."
		continue
	}

	# without a project the editor would come up with its dashboard and the player with an empty
	# scene - that is enough to prove that all DLLs resolve and the window system works
	$extraArgs = switch ($tool)
	{
		"ezEditor.exe" { @("-unattended", "-safe") }
		default { @() }
	}

	$proc = $null

	Invoke-TestCheck -Name "$tool starts" -Check {
		$script:startedProcess = Start-EzProcessDetached -Exe $exe -Arguments $extraArgs -WorkingDirectory $SdkDir
		$p = $script:startedProcess

		# a missing DLL kills the process immediately, a working one keeps running
		if (-not (Wait-ForCondition -TimeoutSeconds $AliveSeconds -PollMilliseconds 500 -Condition { $p.HasExited }))
		{
			return "still running after ${AliveSeconds}s"
		}

		$p.Refresh()
		throw "Exited after $([math]::Round(($p.ExitTime - $p.StartTime).TotalSeconds, 1))s with exit code $($p.ExitCode) (0x$('{0:X8}' -f $p.ExitCode))."
	}

	if (Get-Variable -Name startedProcess -Scope Script -ErrorAction SilentlyContinue)
	{
		$proc = $script:startedProcess
		Remove-Variable -Name startedProcess -Scope Script
	}

	Stop-EzProcessTree -Process $proc
}

# the editor starts a separate engine process, which must not survive the editor
$leftovers = @(Get-LeftoverEzProcesses -BinDir $binDir)

if ($leftovers.Count -gt 0)
{
	# they were killed hard above, so a leftover engine process is expected here and only cleaned up,
	# not reported as a failure - the clean shutdown case is covered by Test-Mcp.ps1
	Write-Host "Cleaning up $($leftovers.Count) leftover process(es)."
	$leftovers | ForEach-Object { Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue }
}

exit (Save-TestResults)
