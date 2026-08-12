# Builds the C++ plugins of a packaged SDK and checks the project export.
#
# This covers what only a package can get wrong: the generated CMake has to find the SDK at the
# path where the package was extracted, not at the path it was built on, and it has to link against
# the import libraries that ship with it.
#
# Requires Visual Studio. Without it the whole group is skipped.
#
# Example:
#   Test-CppProjects.ps1 -SdkDir "D:\ez-test\ezEngine.Release.26.9.0" -OutputDir "D:\ez-test\results"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$SdkDir,
	[Parameter(Mandatory = $true)][string]$OutputDir,
	# explicit binary folder, e.g. a build workspace output; derived from -SdkDir when empty
	[string]$BinDir = "",
	# the samples that have a CppSource folder
	[string[]]$Projects = @("PacMan", "RTS", "Asteroids"),
	[string]$Only = "",
	# a full build of a fresh project can take a while
	[int]$BuildTimeoutSeconds = 1800,
	# skip the (slow) export and the run of the exported project
	[switch]$SkipExport,
	# which project to export; it is transformed and compiled as part of the export
	[string]$ExportProject = "SampleGame",
	# remove each project's CppSource/Build folder before compiling it and after its checks are
	# done, instead of leaving it (and its stale CMake cache) behind in the package for next time
	[switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. "$PSScriptRoot\ReleaseTestCommon.ps1"

$SdkDir = (Resolve-Path $SdkDir).Path
$binDir = Get-EzBinDir -SdkDir $SdkDir -BinDir $BinDir
$processor = Get-EzExe -BinDir $binDir -ExeName "ezEditorProcessor.exe"

Initialize-TestGroup -Group "CppProjects" -OutputDir $OutputDir

$logDir = Join-Path $OutputDir "Logs"
$workDir = Join-Path $OutputDir "CppProjects"
New-Item -ItemType Directory -Force -Path $logDir, $workDir | Out-Null

# --------------------------------------------------------------------------------------

function Test-VisualStudioPresent
{
	$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

	if (-not (Test-Path $vswhere)) { return $false }

	$found = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

	return -not [string]::IsNullOrWhiteSpace($found)
}

function Invoke-EditorProcessor
{
	param([string]$Name, [string[]]$Arguments, [int]$TimeoutSeconds = 1800)

	$logFile = Join-Path $logDir "Cpp-$Name.txt"
	$result = Invoke-EzProcess -Exe $processor -Arguments $Arguments -TimeoutSeconds $TimeoutSeconds -WorkingDirectory $SdkDir
	Set-Content -Path $logFile -Value ($result.StdOut + "`n" + $result.StdErr) -Encoding UTF8

	Add-Member -InputObject $result -NotePropertyName "LogFile" -NotePropertyValue $logFile
	return $result
}

# The exit codes of ezEditorProcessor, see EditorProcessor.cpp.
function Get-ProcessorExitCodeText
{
	param([int]$Code)

	switch ($Code)
	{
		0 { "ok" }
		1 { "transform failed or unknown asset profile" }
		2 { "failed to open the project" }
		3 { "the C++ build failed" }
		4 { "export failed" }
		5 { "project creation failed" }
		200 { "failed to connect to the host process" }
		default { "unknown" }
	}
}

# B3: the generated build files must point at this SDK. If the path rewriting in
# ez_include_ezExport() did not happen, the build folder still references the machine the package
# was built on, which is what EXPINP_SOURCE_DIR in Output/Bin/ezExportInfo.cmake records.
function Test-SdkPathsRelocated
{
	param([string]$ProjectDir)

	$exportInfo = Join-Path $SdkDir "Output/Bin/ezExportInfo.cmake"

	if (-not (Test-Path $exportInfo))
	{
		throw "'$exportInfo' is missing, the SDK cannot be relocated without it."
	}

	$buildPath = ""
	foreach ($line in Get-Content $exportInfo)
	{
		if ($line -match 'set\(EXPINP_SOURCE_DIR\s+(.+?)\)') { $buildPath = $Matches[1].Trim() }
	}

	if (-not $buildPath)
	{
		throw "No EXPINP_SOURCE_DIR in '$exportInfo'."
	}

	$buildDir = Join-Path $ProjectDir "CppSource/Build"

	if (-not (Test-Path $buildDir))
	{
		throw "No build folder was generated below '$ProjectDir'."
	}

	# only the CMake bookkeeping is searched, not the whole build output
	$hits = @(Get-ChildItem $buildDir -Include "*.cmake", "CMakeCache.txt" -Recurse -File -ErrorAction SilentlyContinue |
		Select-String -SimpleMatch -Pattern $buildPath -List | Select-Object -First 3)

	if ($hits.Count -gt 0)
	{
		throw "The build still references the packaging machine's path '$buildPath', e.g. in '$($hits[0].Path)'."
	}

	return "no references to '$buildPath'"
}

# --------------------------------------------------------------------------------------

if (-not (Test-VisualStudioPresent))
{
	Add-TestResult -Name "Visual Studio" -Status "SKIP" -Message "No Visual Studio with the C++ toolset found, the whole group needs a compiler."
	exit (Save-TestResults)
}

$projectsToTest = @($Projects | Where-Object { -not $Only -or $_ -like "*$Only*" })

foreach ($projectName in $projectsToTest)
{
	$projectDir = Join-Path $SdkDir "Data/Samples/$projectName"

	if (-not (Test-Path (Join-Path $projectDir "CppSource")))
	{
		Add-TestResult -Name "$projectName compiles" -Status "SKIP" -Message "No CppSource folder."
		continue
	}

	# CppSource/Build lives inside the package itself (there is no separate workDir for these
	# samples, unlike NewProject below), so unlike a normal build directory it is only removed
	# when asked for via -Clean.
	$buildDir = Join-Path $projectDir "CppSource/Build"
	if ($Clean) { Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue }

	# B1: generate and build the plugin against the packaged SDK
	$result = Invoke-EditorProcessor -Name "$projectName-compile" -TimeoutSeconds $BuildTimeoutSeconds -Arguments @(
		"-project", $projectDir
		"-compile"
		"-outputDir", (Join-Path $workDir $projectName)
	)

	Invoke-TestCheck -Name "$projectName compiles" -Artifact $result.LogFile -DurationSeconds $result.Duration -Check {
		if ($result.TimedOut) { throw "The build did not finish within ${BuildTimeoutSeconds}s." }

		if ($result.ExitCode -ne 0)
		{
			throw "Exit code $($result.ExitCode) ($(Get-ProcessorExitCodeText $result.ExitCode)), see '$($result.LogFile)'."
		}

		$dll = Join-Path $binDir "$($projectName)Plugin.dll"

		if (-not (Test-Path $dll))
		{
			throw "'$dll' was not produced."
		}

		return "$($projectName)Plugin.dll"
	}

	# B3: the relocation of the SDK paths worked
	Invoke-TestCheck -Name "$projectName build uses this SDK" -Check {
		return Test-SdkPathsRelocated -ProjectDir $projectDir
	}

	if ($Clean) { Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue }
}

# B1 (second half): a project created from scratch, which is what a new user does
$newProjectDir = Join-Path $workDir "NewProject"

if (-not $Only)
{
	Remove-Item $newProjectDir -Recurse -Force -ErrorAction SilentlyContinue

	$result = Invoke-EditorProcessor -Name "NewProject-create" -TimeoutSeconds 600 -Arguments @(
		"-createProject", $newProjectDir
		"-pluginTemplate", "General3D"
	)

	Invoke-TestCheck -Name "New project is created" -Artifact $result.LogFile -DurationSeconds $result.Duration -Check {
		if ($result.ExitCode -ne 0)
		{
			throw "Exit code $($result.ExitCode) ($(Get-ProcessorExitCodeText $result.ExitCode))."
		}

		if (-not (Test-Path (Join-Path $newProjectDir "ezProject"))) { throw "No 'ezProject' file was written." }
		if (-not (Test-Path (Join-Path $newProjectDir "Editor/PluginSelection.ddl"))) { throw "No 'Editor/PluginSelection.ddl' was written." }

		return "General3D"
	}

	if (Test-Path (Join-Path $newProjectDir "ezProject"))
	{
		$result = Invoke-EditorProcessor -Name "NewProject-compile" -TimeoutSeconds $BuildTimeoutSeconds -Arguments @(
			"-project", $newProjectDir
			"-compile"
			"-transform", "Default"
			"-outputDir", (Join-Path $workDir "NewProjectOut")
		)

		Invoke-TestCheck -Name "New project compiles and transforms" -Artifact $result.LogFile -DurationSeconds $result.Duration -Check {
			if ($result.TimedOut) { throw "Did not finish within ${BuildTimeoutSeconds}s." }

			if ($result.ExitCode -ne 0)
			{
				throw "Exit code $($result.ExitCode) ($(Get-ProcessorExitCodeText $result.ExitCode)), see '$($result.LogFile)'."
			}

			if (-not (Test-Path (Join-Path $newProjectDir "AssetCache/Default.ezAidlt")))
			{
				throw "No asset cache was written."
			}

			return "built and transformed"
		}

		# B2: a rebuild from scratch, which is the case that catches a stale CMake cache
		Invoke-TestCheck -Name "New project rebuilds from scratch" -Check {
			Remove-Item (Join-Path $newProjectDir "CppSource/Build") -Recurse -Force -ErrorAction SilentlyContinue

			$rebuild = Invoke-EditorProcessor -Name "NewProject-recompile" -TimeoutSeconds $BuildTimeoutSeconds -Arguments @(
				"-project", $newProjectDir
				"-recompile"
				"-outputDir", (Join-Path $workDir "NewProjectOut")
			)

			if ($rebuild.ExitCode -ne 0)
			{
				throw "Exit code $($rebuild.ExitCode) ($(Get-ProcessorExitCodeText $rebuild.ExitCode)), see '$($rebuild.LogFile)'."
			}

			return "recompiled"
		}
	}
}

# B4 + B5: export a project and run the result
if (-not $SkipExport -and -not $Only)
{
	$exportDir = Join-Path $workDir "Export-$ExportProject"
	$exportSource = Join-Path $SdkDir "Data/Samples/$ExportProject"

	$result = Invoke-EditorProcessor -Name "$ExportProject-export" -TimeoutSeconds $BuildTimeoutSeconds -Arguments @(
		"-project", $exportSource
		"-export", $exportDir
	)

	Invoke-TestCheck -Name "$ExportProject exports" -Artifact $result.LogFile -DurationSeconds $result.Duration -Check {
		if ($result.TimedOut) { throw "The export did not finish within ${BuildTimeoutSeconds}s." }

		if ($result.ExitCode -ne 0)
		{
			throw "Exit code $($result.ExitCode) ($(Get-ProcessorExitCodeText $result.ExitCode)), see '$($result.LogFile)'."
		}

		if (-not (Test-Path (Join-Path $exportDir "ExportLog.txt")))
		{
			throw "No 'ExportLog.txt' in '$exportDir'."
		}

		# the MCP plugin bundle declares no runtime plugins, so an exported game must not contain it
		$pluginConfigs = @(Get-ChildItem $exportDir -Filter "Plugins.ddl" -Recurse -File -ErrorAction SilentlyContinue)
		$withMcp = @($pluginConfigs | Select-String -SimpleMatch -Pattern "McpPlugin" -List)

		if ($withMcp.Count -gt 0)
		{
			throw "The exported project references the MCP plugin in '$($withMcp[0].Path)'."
		}

		return "exported to '$exportDir'"
	}

	if (Test-Path $exportDir)
	{
		Invoke-TestCheck -Name "Exported $ExportProject runs" -Check {
			$exe = @(Get-ChildItem $exportDir -Filter "*.exe" -Recurse -File | Where-Object { $_.Name -notlike "ez*Tool*" } | Select-Object -First 1)

			if ($exe.Count -eq 0)
			{
				throw "No executable was exported."
			}

			$screenshot = Join-Path $OutputDir "Screenshots/Export-$ExportProject.png"
			$logFile = Join-Path $logDir "Export-$ExportProject-run.txt"
			Remove-Item $screenshot -ErrorAction SilentlyContinue

			# an exported game only finds its data (and thus its runtime plugin list) via '-project', run
			# the same way as the generated 'Launch Main.bat' does: from the export root, not from Bin/
			$run = Invoke-EzProcess -Exe $exe[0].FullName -TimeoutSeconds 300 -WorkingDirectory $exportDir -Arguments @(
				"-project", "Data/project"
				"-runframes", "60"
				"-timeout", "120"
				"-failonerror"
				"-logfile", $logFile
				"-screenshot", $screenshot
			)

			if ($run.ExitCode -ne 0)
			{
				throw "'$($exe[0].Name)' exited with $($run.ExitCode). $((Get-LogErrors -LogFile $logFile) -join ' | ')"
			}

			if (-not (Test-Path $screenshot))
			{
				# the smoke test options come from the game application, an exported game only has them
				# once they are not player-specific any more
				throw "No screenshot was written - does the exported application support -screenshot?"
			}

			return $exe[0].Name
		}
	}
}

exit (Save-TestResults)
