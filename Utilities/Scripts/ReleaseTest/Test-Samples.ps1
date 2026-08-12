# Runtime smoke test for the sample projects of a packaged SDK.
#
# Starts every sample through ezPlayer, lets it render a number of frames, takes a screenshot
# and checks that the process exited cleanly, logged no errors and rendered something.
#
# Example:
#   Test-Samples.ps1 -SdkDir "D:\ez-test\ezEngine.Release.26.9.0" -OutputDir "D:\ez-test\results"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$SdkDir,
	[Parameter(Mandatory = $true)][string]$OutputDir,
	# explicit binary folder, e.g. a build workspace output; derived from -SdkDir when empty
	[string]$BinDir = "",
	[string[]]$Renderer = @("DX11", "Vulkan"),
	# substring filter on the project name, e.g. -Only PacMan
	[string]$Only = "",
	# test the local sample projects; only useful to turn off when isolating another switch below
	[switch]$IncludeLocal = $true,
	# also download, transform and run the samples that ship as 'ezRemoteProject' stubs
	[switch]$IncludeRemote,
	# run every scene of a project, not only its main scene
	[switch]$AllScenes,
	# also run the standalone sample executables (TextureSample, ShaderExplorer, ...)
	[switch]$IncludeStandalone,
	[int]$RunFrames = 60,
	# the application quits on its own after -runframes; this is only the safety net for a hang
	[int]$TimeoutSeconds = 60,
	# folder with reference screenshots; enables the image comparison when given
	[string]$ReferenceDir = "",
	[double]$MaxImageError = 100.0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. "$PSScriptRoot\ReleaseTestCommon.ps1"

$SdkDir = (Resolve-Path $SdkDir).Path
$binDir = Get-EzBinDir -SdkDir $SdkDir -BinDir $BinDir
$player = Get-EzExe -BinDir $binDir -ExeName "ezPlayer.exe"

Initialize-TestGroup -Group "Samples" -OutputDir $OutputDir

$screenshotDir = Join-Path $OutputDir "Screenshots"
$logDir = Join-Path $OutputDir "Logs"
New-Item -ItemType Directory -Force -Path $screenshotDir, $logDir | Out-Null

# --------------------------------------------------------------------------------------

function Get-ProjectScenes
{
	param([string]$ProjectDir, [bool]$All)

	$sceneDir = Join-Path $ProjectDir "Scenes"
	$searchDir = if (Test-Path $sceneDir) { $sceneDir } else { $ProjectDir }

	# only the top level, the '<scene>_data' subfolders contain prefabs and layers, not startable scenes
	$scenes = @(Get-ChildItem -Path $searchDir -Filter "*.ezScene" -File | Sort-Object Name)

	if (-not $All)
	{
		$main = $scenes | Where-Object { $_.BaseName -eq "Main" } | Select-Object -First 1
		if ($main) { return @($main) }
		return @($scenes | Select-Object -First 1)
	}

	return $scenes
}

# Runs one scene of one project on one renderer and evaluates exit code, log and screenshot.
function Test-Scene
{
	param([string]$Name, [string]$ProjectDir, [string]$ScenePath, [string]$RendererName)

	$safeName = ($Name -replace '[^\w\-]', '_')
	$screenshot = Join-Path $screenshotDir "$safeName-$RendererName.png"
	$logFile = Join-Path $logDir "$safeName-$RendererName.txt"

	Remove-Item $screenshot -ErrorAction SilentlyContinue

	$playerArgs = @(
		"-project", $ProjectDir
		"-scene", $ScenePath
		"-renderer", $RendererName
		"-runframes", "$RunFrames"
		"-timeout", "$TimeoutSeconds"
		"-failonerror"
		"-logfile", $logFile
		"-screenshot", $screenshot
	)

	# the player enforces -timeout itself, this is only the safety net for a hang before that logic runs
	$result = Invoke-EzProcess -Exe $player -Arguments $playerArgs -TimeoutSeconds ($TimeoutSeconds + 30)

	# A1, A2, A5: started, rendered, exited cleanly, logged no errors
	Invoke-TestCheck -Name "$Name [$RendererName] runs" -Artifact $logFile -DurationSeconds $result.Duration -Check {
		if ($result.TimedOut)
		{
			throw "Process had to be killed after $($TimeoutSeconds + 30)s - it ignored its own -timeout."
		}

		if ($result.ExitCode -ne 0)
		{
			$logErrors = (Get-LogErrors -LogFile $logFile) -join " | "
			$reason = switch ($result.ExitCode)
			{
				1 { "errors were logged" }
				2 { "the player hit its own -timeout" }
				default { "unexpected exit code" }
			}
			throw "Exit code $($result.ExitCode) ($reason). $logErrors"
		}

		return "$RunFrames frames"
	}

	# A3: the screenshot exists and is not a single flat color
	Invoke-TestCheck -Name "$Name [$RendererName] renders" -Artifact $screenshot -Check {
		if (-not (Test-Path $screenshot))
		{
			throw "No screenshot was written."
		}

		$stats = Get-ImageLuminanceStats -Path $screenshot

		# an empty scene still renders a sky and some fog gradient, a broken renderer produces one flat color
		if ($stats.StdDev -lt 1.0)
		{
			throw ("Screenshot is a flat color (mean {0:N1}, stddev {1:N2}) - the project started but rendered nothing." -f $stats.Mean, $stats.StdDev)
		}

		return "{0}x{1}, stddev {2:N1}" -f $stats.Width, $stats.Height, $stats.StdDev
	}

	# A4: comparison against a reference image, only when references are available
	if ($ReferenceDir)
	{
		$reference = Join-Path $ReferenceDir "$safeName-$RendererName.png"

		if (-not (Test-Path $reference))
		{
			Add-TestResult -Name "$Name [$RendererName] matches reference" -Status "SKIP" -Message "No reference image '$reference'."
		}
		else
		{
			Invoke-TestCheck -Name "$Name [$RendererName] matches reference" -Artifact $screenshot -Check {
				$mse = Get-ImageMeanSquareError -PathA $screenshot -PathB $reference

				if ($mse -gt $MaxImageError)
				{
					throw ("Image differs from the reference (MSE {0:N1} > {1:N1})." -f $mse, $MaxImageError)
				}

				return "MSE {0:N1}" -f $mse
			}
		}
	}
}

function Test-Project
{
	param([string]$Name, [string]$ProjectDir)

	# the array subexpression is required, a single returned scene would otherwise be unrolled
	$scenes = @(Get-ProjectScenes -ProjectDir $ProjectDir -All:$AllScenes.IsPresent)

	if ($scenes.Count -eq 0)
	{
		Add-TestResult -Name $Name -Status "SKIP" -Message "No scene file found in '$ProjectDir'."
		return
	}

	foreach ($scene in $scenes)
	{
		# relative to the project data directory, which is how the player expects it
		$relative = $scene.FullName.Substring($ProjectDir.Length).TrimStart('\', '/').Replace('\', '/')
		$sceneName = if ($scenes.Count -eq 1) { $Name } else { "$Name/$($scene.BaseName)" }

		foreach ($r in $Renderer)
		{
			Test-Scene -Name $sceneName -ProjectDir $ProjectDir -ScenePath $relative -RendererName $r
		}
	}
}

# A8: check out a remote project through the editor, so that the git clone, the archive
# extraction and the redirection file are all exercised, then transform its assets.
function Initialize-RemoteProject
{
	param([string]$Name, [string]$StubDir)

	$processor = Get-EzExe -BinDir $binDir -ExeName "ezEditorProcessor.exe"
	$checkoutRoot = Join-Path $OutputDir "RemoteProjects"
	New-Item -ItemType Directory -Force -Path $checkoutRoot | Out-Null

	$projectDir = Join-Path $checkoutRoot $Name
	$transformLog = Join-Path $logDir "$Name-checkout.txt"

	# the stub file itself, not its folder - that is what the editor recognizes as a remote project
	$result = Invoke-EzProcess -Exe $processor -TimeoutSeconds 3600 -Arguments @(
		"-project", (Join-Path $StubDir "ezRemoteProject")
		"-remoteProjectDir", $checkoutRoot
		"-transform", "Default"
		"-outputDir", (Join-Path $OutputDir "EditorProcessor/$Name")
	)

	Set-Content -Path $transformLog -Value ($result.StdOut + "`n" + $result.StdErr) -Encoding UTF8

	if ($result.ExitCode -ne 0)
	{
		throw "Checkout and transform failed with exit code $($result.ExitCode), see '$transformLog'."
	}

	# not every remote repo puts 'ezProject' at its root - e.g. the Monster Attack repo nests the
	# actual project one folder deeper (it also carries an 'ezEngine' submodule at its root). Check
	# the common flat case first, only fall back to a recursive search (which would otherwise also
	# scan straight through that submodule) if that comes up empty.
	$flatProjectFile = Join-Path $projectDir "ezProject"
	$projectFile = if (Test-Path $flatProjectFile) { Get-Item $flatProjectFile } else { Get-ChildItem -Path $projectDir -Filter "ezProject" -Recurse -File -ErrorAction SilentlyContinue | Select-Object -First 1 }

	if (-not $projectFile)
	{
		throw "No project was checked out to '$projectDir'."
	}

	return $projectFile.DirectoryName
}

# --------------------------------------------------------------------------------------

$samplesDir = Join-Path $SdkDir "Data/Samples"

$localProjects = @(Get-ChildItem -Path $samplesDir -Directory | Where-Object {
	Test-Path (Join-Path $_.FullName "ezProject")
})

$remoteProjects = @(Get-ChildItem -Path $samplesDir -Directory | Where-Object {
	Test-Path (Join-Path $_.FullName "ezRemoteProject")
})

if ($Only)
{
	$localProjects = @($localProjects | Where-Object { $_.Name -like "*$Only*" })
	$remoteProjects = @($remoteProjects | Where-Object { $_.Name -like "*$Only*" })
}

Write-Host ("Found {0} local and {1} remote sample projects in '{2}'." -f $localProjects.Count, $remoteProjects.Count, $samplesDir)

if ($IncludeLocal)
{
	foreach ($project in $localProjects)
	{
		Test-Project -Name $project.Name -ProjectDir $project.FullName
	}
}

foreach ($project in $remoteProjects)
{
	if (-not $IncludeRemote)
	{
		Add-TestResult -Name $project.Name -Status "SKIP" -Message "Remote project, pass -IncludeRemote to download and run it."
		continue
	}

	$projectDir = $null
	$sw = [System.Diagnostics.Stopwatch]::StartNew()

	try
	{
		$projectDir = Initialize-RemoteProject -Name $project.Name -StubDir $project.FullName
		$sw.Stop()
		Add-TestResult -Name "$($project.Name) checkout and transform" -Status "PASS" -DurationSeconds $sw.Elapsed.TotalSeconds -Message "Checked out to '$projectDir'."
	}
	catch
	{
		$sw.Stop()
		Add-TestResult -Name "$($project.Name) checkout and transform" -Status "FAIL" -DurationSeconds $sw.Elapsed.TotalSeconds -Message $_.Exception.Message
	}

	if ($projectDir)
	{
		Test-Project -Name $project.Name -ProjectDir $projectDir
	}
}

# A6: the standalone sample applications have no project and no scene, they only need to start and
# not crash.
#
# These four derive from ezApplication directly, not from ezGameApplication, so they do not have
# the unattended options ('-runframes', '-screenshot', '-timeout', ...) - passing those is pointless,
# and worse, waiting out Invoke-EzProcess's own timeout for a process that never reacts to '-timeout'
# takes up to $TimeoutSeconds+30s per app with a GUI window sitting on screen the whole time, which
# looks and feels like the script hanging. Use the same bounded start/kill pattern as Test-Tools.ps1
# instead: start it, give it a few seconds to prove it didn't crash on startup, then hard-kill it.
if ($IncludeStandalone)
{
	foreach ($exeName in @("TextureSample.exe", "MeshRenderSample.exe", "ShaderExplorer.exe", "ComputeShaderHistogram.exe"))
	{
		$name = [System.IO.Path]::GetFileNameWithoutExtension($exeName)

		if ($Only -and $name -notlike "*$Only*") { continue }

		$exePath = Join-Path $binDir $exeName

		if (-not (Test-Path $exePath))
		{
			Add-TestResult -Name $name -Status "FAIL" -Message "'$exeName' is missing from the package."
			continue
		}

		$logFile = Join-Path $logDir "$name.txt"
		$proc = $null

		Invoke-TestCheck -Name "$name runs" -Artifact $logFile -Check {
			$script:startedProcess = Start-EzProcessDetached -Exe $exePath
			$p = $script:startedProcess

			if (Wait-ForCondition -TimeoutSeconds 5 -PollMilliseconds 500 -Condition { $p.HasExited })
			{
				$p.Refresh()
				throw "Exited after $([math]::Round(($p.ExitTime - $p.StartTime).TotalSeconds, 1))s with exit code $($p.ExitCode) (0x$('{0:X8}' -f $p.ExitCode))."
			}

			return "still running after 5s"
		}

		if (Get-Variable -Name startedProcess -Scope Script -ErrorAction SilentlyContinue)
		{
			$proc = $script:startedProcess
			Remove-Variable -Name startedProcess -Scope Script
		}

		Stop-EzProcessTree -Process $proc
		Save-DetachedProcessOutput -Process $proc -LogFile $logFile
	}
}

exit (Save-TestResults)
