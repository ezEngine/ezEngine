# Runs the release test suite against a packaged ezEngine SDK and writes a summary.
#
# This is meant to be run on a normal PC with the downloaded package, not on a build server:
# it starts the shipped applications, so it needs a GPU, a desktop session and, for the C++ group,
# a Visual Studio installation.
#
# Each -Cpp/-Mcp/-Tools/-EditorSamples*/-StandaloneSamples switch below ADDS that group to the run.
# Give none of them and everything runs, without exception (including the slow remote-project
# checkout and the full-scene sample sweep, which otherwise only run when asked for).
#
# Example:
#   Run-ReleaseTests.ps1 -SdkDir "D:\ez-test\ezEngine.Release.26.9.0" -OutputDir "D:\ez-test\results-26.9.0"
#   Run-ReleaseTests.ps1 -SdkDir ... -OutputDir ... -EditorSamplesRemote   # only the remote projects
#
# The exit code is the number of failed checks.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$SdkDir,
	[Parameter(Mandatory = $true)][string]$OutputDir,
	# explicit binary folder, e.g. a build workspace output; derived from -SdkDir when empty
	[string]$BinDir = "",
	# the CppProjects group: builds the C++ samples and exports/runs one of them
	[switch]$Cpp,
	# the Mcp group: smoke-tests the editor's and player's MCP servers
	[switch]$Mcp,
	# the Tools group: starts every shipped tool
	[switch]$Tools,
	# runs the local sample projects' main scene
	[switch]$EditorSamplesLocal,
	# modifier only: runs every scene instead of just the main scene, for whichever of
	# EditorSamplesLocal/EditorSamplesRemote is also given. It selects no group by itself, so giving
	# only this switch is not a selective run - everything runs, with every scene.
	[switch]$EditorSamplesAllScenes,
	# downloads, transforms and runs the samples that ship as remote projects (slow, several GB)
	[switch]$EditorSamplesRemote,
	# runs the standalone sample executables (TextureSample, ShaderExplorer, ...)
	[switch]$StandaloneSamples,
	[ValidateSet("DX11", "Vulkan", "All")][string[]]$Renderer = @("All"),
	# skip the project export and the run of the exported project, within the CppProjects group
	[switch]$SkipExport,
	[string]$ReferenceDir = "",
	# remove everything this run (and any earlier run against the same -OutputDir) added to the
	# package: a plugin DLL/PDB compiled into the binary folder, a sample's CppSource/Build folder,
	# anything else a test wrote into -SdkDir. See 'PackageBaseline.txt' below for how this is
	# tracked. Off by default so the package is left as-is for inspection after a run.
	[switch]$Clean,
	# report what -Clean would remove, without removing it and without running any checks. Requires
	# a baseline already recorded for -OutputDir (see 'PackageBaseline.txt' below); errors out if
	# there isn't one yet instead of running the suite to create it.
	[switch]$ListNewFiles
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. "$PSScriptRoot\ReleaseTestCommon.ps1"

$SdkDir = (Resolve-Path $SdkDir).Path
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path

# not '$binDir': PowerShell variable names are case insensitive, so that would silently overwrite the
# -BinDir parameter and pass the resolved path on to the sub-scripts instead of what the caller gave
$resolvedBinDir = Get-EzBinDir -SdkDir $SdkDir -BinDir $BinDir
$version = "unknown"
$versionFile = Join-Path $SdkDir "version.txt"
if (Test-Path $versionFile) { $version = (Get-Content $versionFile -Raw).Trim() }

Write-Host "ezEngine release test"
Write-Host "  SDK:     $SdkDir (version $version)"
Write-Host "  Binaries: $resolvedBinDir"
Write-Host "  Output:  $OutputDir"

# --------------------------------------------------------------------------------------
# Package baseline, for -Clean and -ListNewFiles
# --------------------------------------------------------------------------------------

# Every file below -SdkDir, as forward-slash relative paths. Building this once and diffing
# against it later is what lets -Clean tell a file the package shipped with apart from one a test
# wrote into it - the package itself has no such list.
function Get-SdkFileList
{
	Get-ChildItem $SdkDir -Recurse -File | ForEach-Object { $_.FullName.Substring($SdkDir.Length + 1).Replace('\', '/') }
}

# Diffs $CurrentPaths against $BaselinePaths and returns the new entries, collapsing an entirely
# new folder into a single "folder/" entry instead of listing every file underneath it.
function Get-NewEntries
{
	param([string[]]$BaselinePaths, [string[]]$CurrentPaths)

	$baselineFiles = [System.Collections.Generic.HashSet[string]]::new([string[]]$BaselinePaths)

	# every directory that already existed, at every nesting level - a new file directly below
	# one of these is just a new file; a new file below anything else sits in a wholly new folder
	$baselineDirs = [System.Collections.Generic.HashSet[string]]::new()
	foreach ($p in $BaselinePaths)
	{
		$parts = $p -split '/'
		$dir = ""
		for ($i = 0; $i -lt $parts.Count - 1; $i++)
		{
			$dir = if ($dir) { "$dir/$($parts[$i])" } else { $parts[$i] }
			$baselineDirs.Add($dir) | Out-Null
		}
	}

	$newFiles = @($CurrentPaths | Where-Object { -not $baselineFiles.Contains($_) })
	$reported = [System.Collections.Generic.HashSet[string]]::new()
	$entries = [System.Collections.Generic.List[string]]::new()

	foreach ($p in $newFiles)
	{
		$parts = $p -split '/'
		$dir = ""
		$newRoot = $null

		for ($i = 0; $i -lt $parts.Count - 1; $i++)
		{
			$dir = if ($dir) { "$dir/$($parts[$i])" } else { $parts[$i] }
			if (-not $baselineDirs.Contains($dir)) { $newRoot = $dir; break }
		}

		$entry = if ($newRoot) { "$newRoot/" } else { $p }

		if ($reported.Add($entry)) { $entries.Add($entry) }
	}

	return @($entries | Sort-Object)
}

$baselineFile = Join-Path $OutputDir "PackageBaseline.txt"

# -ListNewFiles only ever reports; it never runs a check and never creates the baseline it reads -
# without an existing baseline there is nothing to diff against, so that is an error, not a run.
if ($ListNewFiles)
{
	if (-not (Test-Path $baselineFile))
	{
		# Write-Host, not Write-Error: with $ErrorActionPreference = 'Stop' the latter throws and the
		# explicit exit code below would never be reached
		Write-Host "No package baseline recorded yet at '$baselineFile'. Run the suite once for this -OutputDir (with or without -Clean) to record one, then -ListNewFiles can diff against it." -ForegroundColor Red
		exit 1
	}

	$baseline = @(Get-Content $baselineFile)
	$newEntries = Get-NewEntries -BaselinePaths $baseline -CurrentPaths @(Get-SdkFileList)

	if ($newEntries.Count -eq 0)
	{
		Write-Host "No files were added to '$SdkDir' since the baseline was recorded."
	}
	else
	{
		Write-Host ("New since baseline (pass -Clean to remove): {0} item(s) below '{1}':" -f $newEntries.Count, $SdkDir)
		foreach ($e in $newEntries) { Write-Host "  $e" }
	}

	exit 0
}

if (-not (Test-Path $baselineFile))
{
	Write-Host "No package baseline yet for this output folder, recording the current contents of '$SdkDir'..."
	Get-SdkFileList | Set-Content -Path $baselineFile -Encoding UTF8
}

# Any group switch given at all -> selective mode, only the given groups run; none given -> everything
# runs. -EditorSamplesAllScenes is deliberately not part of this: it is a modifier, not a group, and
# counting it here would turn '-EditorSamplesAllScenes' on its own into a run that selects no group at
# all and then reports success without having checked anything.
$anySelected = $Cpp -or $Mcp -or $Tools -or $EditorSamplesLocal -or $EditorSamplesRemote -or $StandaloneSamples

$runCpp = if ($anySelected) { $Cpp.IsPresent } else { $true }
$runMcp = if ($anySelected) { $Mcp.IsPresent } else { $true }
$runTools = if ($anySelected) { $Tools.IsPresent } else { $true }
$runAllScenes = if ($anySelected) { $EditorSamplesAllScenes.IsPresent } else { $true }
$runLocalSamples = if ($anySelected) { $EditorSamplesLocal.IsPresent } else { $true }
$runRemoteSamples = if ($anySelected) { $EditorSamplesRemote.IsPresent } else { $true }
$runStandaloneSamples = if ($anySelected) { $StandaloneSamples.IsPresent } else { $true }
$runSamples = $runLocalSamples -or $runRemoteSamples -or $runStandaloneSamples

$rendererList = if ($Renderer -contains "All") { @("DX11", "Vulkan") } else { $Renderer }

$groups = @()
if ($runTools) { $groups += "Tools" }
if ($runSamples) { $groups += "Samples" }
if ($runCpp) { $groups += "CppProjects" }
if ($runMcp) { $groups += "Mcp" }

# a run that selects nothing must not end in a green '0 checks, 0 failed'
if ($groups.Count -eq 0)
{
	Write-Host "The given switches select no test group, there is nothing to run." -ForegroundColor Red
	exit 1
}

# old results would otherwise be merged into the new summary
Get-ChildItem $OutputDir -Filter "Results.*.json" -File -ErrorAction SilentlyContinue | Remove-Item -Force

# --------------------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------------------

# Rebuilds Summary.md and Results.json from whatever Results.<Group>.json files exist so far.
# Called after every group (not just at the end) so a run that hangs or is killed midway still
# leaves a readable summary behind instead of only the per-group JSON.
function Update-Summary
{
	$all = [System.Collections.Generic.List[object]]::new()

	foreach ($file in Get-ChildItem $OutputDir -Filter "Results.*.json" -File)
	{
		# ConvertFrom-Json writes its whole result as a single pipeline object, which for a JSON array
		# already *is* that array - wrapping the pipe itself in @() would collect that one object into
		# a further array, nesting it. Assign first, then @() the variable to also cover the case of a
		# results file with exactly one entry (where ConvertFrom-Json returns a scalar, not an array).
		$parsed = Get-Content $file.FullName -Raw | ConvertFrom-Json
		foreach ($item in @($parsed)) { $all.Add($item) }
	}

	$failed = @($all | Where-Object { $_.Status -eq "FAIL" })
	$skipped = @($all | Where-Object { $_.Status -eq "SKIP" })

	$lines = @()
	$lines += "# ezEngine release test - $version"
	$lines += ""
	$lines += "- SDK: ``$SdkDir``"
	$lines += "- Binaries: ``$(Split-Path $resolvedBinDir -Leaf)``"
	$lines += "- Run: $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
	$lines += "- Result: **$($all.Count) checks, $($failed.Count) failed, $($skipped.Count) skipped**"
	$lines += ""

	if ($failed.Count -gt 0)
	{
		$lines += "## Failed"
		$lines += ""
		foreach ($r in $failed)
		{
			$lines += "- **$($r.Group) / $($r.Name)** - $($r.Message)"
		}
		$lines += ""
	}

	foreach ($group in ($all | ForEach-Object { $_.Group } | Select-Object -Unique))
	{
		$lines += "## $group"
		$lines += ""
		$lines += "| Status | Check | Time | Details |"
		$lines += "|---|---|---|---|"

		foreach ($r in ($all | Where-Object { $_.Group -eq $group }))
		{
			$details = $r.Message
			if ($r.Artifact) { $details += " ``$($r.Artifact)``" }
			$lines += "| $($r.Status) | $($r.Name) | $($r.Duration)s | $details |"
		}

		$lines += ""
	}

	$summaryFile = Join-Path $OutputDir "Summary.md"
	Set-Content -Path $summaryFile -Value $lines -Encoding UTF8
	ConvertTo-Json -InputObject @($all) -Depth 4 | Set-Content -Path (Join-Path $OutputDir "Results.json") -Encoding UTF8

	return [PSCustomObject]@{ All = $all; Failed = $failed; Skipped = $skipped; SummaryFile = $summaryFile }
}

foreach ($group in $groups)
{
	$sw = [System.Diagnostics.Stopwatch]::StartNew()

	try
	{
		switch ($group)
		{
			"Tools"
			{
				& "$PSScriptRoot\Test-Tools.ps1" -SdkDir $SdkDir -OutputDir $OutputDir -BinDir $BinDir
			}
			"Samples"
			{
				& "$PSScriptRoot\Test-Samples.ps1" -SdkDir $SdkDir -OutputDir $OutputDir -BinDir $BinDir -Renderer $rendererList `
					-IncludeLocal:$runLocalSamples -IncludeRemote:$runRemoteSamples -AllScenes:$runAllScenes -IncludeStandalone:$runStandaloneSamples -ReferenceDir $ReferenceDir
			}
			"CppProjects"
			{
				& "$PSScriptRoot\Test-CppProjects.ps1" -SdkDir $SdkDir -OutputDir $OutputDir -BinDir $BinDir -SkipExport:$SkipExport -Clean:$Clean
			}
			"Mcp"
			{
				& "$PSScriptRoot\Test-Mcp.ps1" -SdkDir $SdkDir -OutputDir $OutputDir -BinDir $BinDir
			}
		}
	}
	catch
	{
		# a group that dies half way still leaves its results file behind, the summary reports what it got
		Write-Host "Group '$group' aborted: $($_.Exception.Message)" -ForegroundColor Red
	}

	$sw.Stop()
	Write-Host ("Group '{0}' took {1:N0}s." -f $group, $sw.Elapsed.TotalSeconds)

	$result = Update-Summary
	Write-Host ("Summary updated: {0} checks, {1} failed, {2} skipped so far." -f $result.All.Count, $result.Failed.Count, $result.Skipped.Count)
}

$result = Update-Summary

if ($Clean)
{
	$baseline = @(Get-Content $baselineFile)
	$newEntries = Get-NewEntries -BaselinePaths $baseline -CurrentPaths @(Get-SdkFileList)

	if ($newEntries.Count -eq 0)
	{
		Write-Host "No files were added to '$SdkDir' since the baseline was recorded."
	}
	else
	{
		Write-Host ("Removing {0} item(s) below '{1}':" -f $newEntries.Count, $SdkDir)
		foreach ($e in $newEntries)
		{
			Write-Host "  $e"
			$path = Join-Path $SdkDir ($e.TrimEnd('/') -replace '/', '\')
			Remove-Item $path -Recurse -Force -ErrorAction SilentlyContinue
		}
	}
}

Write-Host ""
$summaryColor = if ($result.Failed.Count -gt 0) { "Red" } else { "Green" }
Write-Host ("{0} checks, {1} failed, {2} skipped" -f $result.All.Count, $result.Failed.Count, $result.Skipped.Count) -ForegroundColor $summaryColor
Write-Host "Summary: $($result.SummaryFile)"

exit $result.Failed.Count
