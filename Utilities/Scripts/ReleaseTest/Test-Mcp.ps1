# Smoke test for the MCP servers of the packaged editor and player.
#
# Launches the editor with a project, waits for its MCP port, walks through the project with a few
# tools, does the same on the engine process, and shuts everything down through 'app_quit'.
#
# Example:
#   Test-Mcp.ps1 -SdkDir "D:\ez-test\ezEngine.Release.26.9.0" -OutputDir "D:\ez-test\results"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$SdkDir,
	[Parameter(Mandatory = $true)][string]$OutputDir,
	# explicit binary folder, e.g. a build workspace output; derived from -SdkDir when empty
	[string]$BinDir = "",
	[string]$Project = "Data/Samples/Testing Chambers",
	[string]$Scene = "Scenes/Main.ezScene",
	# not the default port 7391, so that an editor the user has open is never talked to by accident
	[int]$EditorPort = 7399,
	[int]$StartupTimeoutSeconds = 90
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. "$PSScriptRoot\ReleaseTestCommon.ps1"

$SdkDir = (Resolve-Path $SdkDir).Path
$binDir = Get-EzBinDir -SdkDir $SdkDir -BinDir $BinDir
$editorExe = Get-EzExe -BinDir $binDir -ExeName "ezEditor.exe"
$playerExe = Get-EzExe -BinDir $binDir -ExeName "ezPlayer.exe"

$projectDir = Join-Path $SdkDir $Project

Initialize-TestGroup -Group "Mcp" -OutputDir $OutputDir

$screenshotDir = Join-Path $OutputDir "Screenshots"
$logDir = Join-Path $OutputDir "Logs"
New-Item -ItemType Directory -Force -Path $screenshotDir, $logDir | Out-Null

# without this the editor would be started anyway and the only symptom would be a startup timeout
if (-not (Test-Path (Join-Path $projectDir "ezProject")))
{
	Add-TestResult -Name "Project exists" -Status "FAIL" -Message "'$projectDir' contains no 'ezProject' file."
	exit (Save-TestResults)
}

# tools that must exist in every build, regardless of which plugins are loaded
$expectedEditorTools = @("app_info", "app_ping", "cvar_list", "rtti_find_types", "project_info",
	"asset_find", "document_open", "document_list", "object_tree", "selection_set", "action_execute")
$expectedGameTools = @("app_info", "app_screenshot", "game_info", "game_wait", "input_slots", "input_sequence")

$editorProcess = $null
$playerProcess = $null
$editorLog = Join-Path $logDir "Mcp-Editor.txt"
$playerLog = Join-Path $logDir "Mcp-Player.txt"

try
{
	# D1: the editor starts and its MCP server answers
	Invoke-TestCheck -Name "Editor MCP server starts" -Artifact $editorLog -Check {
		if (Test-McpPortOpen -Port $EditorPort)
		{
			throw "Port $EditorPort is already in use - is another editor running?"
		}

		# -unattended is required: the MCP call wrapper suppresses dialogs during tool calls, but not
		# the ones that can appear while the project is being opened.
		# Do not add -safe: in safe mode no project is opened, and the MCP server only starts once a
		# project is open (ToolsProjectEventHandler in EditorPluginMcp).
		$script:editorProc = Start-EzProcessDetached -Exe $editorExe -WorkingDirectory $SdkDir -Arguments @(
			"-project", $projectDir
			"-unattended"
			"-editor-mcpport", "$EditorPort"
		)

		if (-not (Wait-ForCondition -TimeoutSeconds $StartupTimeoutSeconds -Condition { Test-McpPortOpen -Port $EditorPort }))
		{
			throw "The editor did not open port $EditorPort within ${StartupTimeoutSeconds}s."
		}

		$script:editorReady = $true
		return "port $EditorPort"
	}

	if (Get-Variable -Name editorProc -Scope Script -ErrorAction SilentlyContinue)
	{
		$editorProcess = $script:editorProc
		Remove-Variable -Name editorProc -Scope Script
	}

	$editorReachable = [bool](Get-Variable -Name editorReady -Scope Script -ValueOnly -ErrorAction SilentlyContinue)
	Remove-Variable -Name editorReady -Scope Script -ErrorAction SilentlyContinue

	# Every check below talks to that port with a 300s timeout, so continuing without a server that
	# answers does not produce more information - it just adds an hour of guaranteed timeouts.
	if (-not $editorReachable)
	{
		exit (Save-TestResults)
	}

	# D2: protocol handshake and tool inventory
	Invoke-TestCheck -Name "initialize and tools/list" -Check {
		$init = Invoke-McpRequest -Port $EditorPort -Method "initialize" -TimeoutSeconds 300 -Params @{
			protocolVersion = "2025-06-18"
			capabilities    = @{}
			clientInfo      = @{ name = "ezReleaseTest"; version = "1.0" }
		}

		if (-not $init.protocolVersion)
		{
			throw "'initialize' did not report a protocol version."
		}

		$tools = Invoke-McpRequest -Port $EditorPort -Method "tools/list" -TimeoutSeconds 300
		$names = @($tools.tools | ForEach-Object { $_.name })
		$missing = @($expectedEditorTools | Where-Object { $names -notcontains $_ })

		if ($missing.Count -gt 0)
		{
			throw "These tools are missing: $($missing -join ', ')"
		}

		# every tool has to describe its arguments, otherwise a client cannot call it
		$noSchema = @($tools.tools | Where-Object { $null -eq $_.inputSchema } | ForEach-Object { $_.name })

		if ($noSchema.Count -gt 0)
		{
			throw "These tools have no input schema: $($noSchema -join ', ')"
		}

		$listFile = Join-Path $OutputDir "McpEditorTools.json"
		ConvertTo-Json -InputObject @($names | Sort-Object) -Depth 3 | Set-Content -Path $listFile -Encoding UTF8

		return "$($names.Count) tools, protocol $($init.protocolVersion)"
	} -Artifact (Join-Path $OutputDir "McpEditorTools.json")

	# D3: look around in the open project
	Invoke-TestCheck -Name "project_info" -Check {
		$info = Get-McpToolJson -Port $EditorPort -Tool "project_info" -TimeoutSeconds 300

		if (-not $info.projectDirectory)
		{
			throw "No project directory was reported - is a project open?"
		}

		return $info.projectDirectory
	}

	Invoke-TestCheck -Name "asset_find" -Check {
		$found = Get-McpToolJson -Port $EditorPort -Tool "asset_find" -Arguments @{ name = "Main" } -TimeoutSeconds 300

		if ($found -is [string])
		{
			throw "Unexpected response: $found"
		}

		return "found assets for 'Main'"
	}

	Invoke-TestCheck -Name "document_open and object_tree" -Check {
		Invoke-McpTool -Port $EditorPort -Tool "document_open" -Arguments @{ path = $Scene } -TimeoutSeconds 600 | Out-Null

		$tree = Get-McpToolJson -Port $EditorPort -Tool "object_tree" -Arguments @{ document = $Scene } -TimeoutSeconds 300

		if ($tree -is [string])
		{
			throw "object_tree returned no structured data: $tree"
		}

		return "scene opened"
	}

	Invoke-TestCheck -Name "selection_set and selection_get" -Check {
		$tree = Get-McpToolJson -Port $EditorPort -Tool "object_tree" -Arguments @{ document = $Scene } -TimeoutSeconds 300

		# the payload shape differs between tool versions, so pick the first uuid that appears anywhere
		$json = $tree | ConvertTo-Json -Depth 20
		$match = [regex]::Match($json, '[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}')

		if (-not $match.Success)
		{
			throw "No object guid found in the scene tree."
		}

		Invoke-McpTool -Port $EditorPort -Tool "selection_set" -Arguments @{ document = $Scene; objects = @($match.Value) } -TimeoutSeconds 300 | Out-Null
		Get-McpToolJson -Port $EditorPort -Tool "selection_get" -Arguments @{ document = $Scene } -TimeoutSeconds 300 | Out-Null

		return "selected $($match.Value)"
	}

	# D4: the engine process behind the editor answers on its own port
	$enginePort = 0

	Invoke-TestCheck -Name "app_info reports the engine port" -Check {
		$info = Get-McpToolJson -Port $EditorPort -Tool "app_info" -TimeoutSeconds 300

		# the value is the string 'unknown' when the editor could not determine the port of its engine process
		$port = 0
		if (-not [int]::TryParse([string]$info.engineMcpPort, [ref]$port) -or $port -le 0)
		{
			throw "app_info reported no usable engine port: '$($info.engineMcpPort)'."
		}

		$script:foundEnginePort = $port
		return "engine port $port, pid $($info.processId)"
	}

	if (Get-Variable -Name foundEnginePort -Scope Script -ErrorAction SilentlyContinue)
	{
		$enginePort = $script:foundEnginePort
		Remove-Variable -Name foundEnginePort -Scope Script
	}

	if ($enginePort -gt 0)
	{
		Invoke-TestCheck -Name "Engine process MCP server answers" -Check {
			if (-not (Wait-ForCondition -TimeoutSeconds 60 -Condition { Test-McpPortOpen -Port $enginePort }))
			{
				throw "Port $enginePort is not open."
			}

			$tools = Invoke-McpRequest -Port $enginePort -Method "tools/list" -TimeoutSeconds 300
			$names = @($tools.tools | ForEach-Object { $_.name })
			$missing = @($expectedGameTools | Where-Object { $names -notcontains $_ })

			if ($missing.Count -gt 0)
			{
				throw "These game tools are missing: $($missing -join ', ')"
			}

			return "$($names.Count) tools"
		}

		# play-in-editor, which is also what makes the engine process render continuously -
		# without it app_screenshot has nothing to capture
		Invoke-TestCheck -Name "action_execute Scene.GameMode.Play" -Check {
			Invoke-McpTool -Port $EditorPort -Tool "action_execute" -TimeoutSeconds 300 `
				-Arguments @{ document = $Scene; name = "Scene.GameMode.Play" } | Out-Null

			Get-McpToolJson -Port $enginePort -Tool "game_wait" -Arguments @{ frames = 30; timeout = 60 } -TimeoutSeconds 120 | Out-Null

			return "game mode running"
		}

		$screenshotTarget = Join-Path $screenshotDir "Mcp-EngineView.png"

		Invoke-TestCheck -Name "app_screenshot of the running game" -Artifact $screenshotTarget -Check {
			Remove-Item $screenshotTarget -ErrorAction SilentlyContinue

			# an explicit path, otherwise the file lands under a generated name in the temp folder
			$result = Get-McpToolJson -Port $enginePort -Tool "app_screenshot" -TimeoutSeconds 300 `
				-Arguments @{ path = $screenshotTarget }

			if (-not (Test-Path $screenshotTarget))
			{
				throw "No screenshot was written. Response: $($result | ConvertTo-Json -Depth 10 -Compress)"
			}

			$stats = Get-ImageLuminanceStats -Path $screenshotTarget

			if ($stats.StdDev -lt 1.0)
			{
				throw ("The captured frame is a flat color (stddev {0:N2})." -f $stats.StdDev)
			}

			return "{0}x{1}, stddev {2:N1}" -f $stats.Width, $stats.Height, $stats.StdDev
		}
	}

	# D5: everything shuts down through MCP, without leaving processes behind
	Invoke-TestCheck -Name "app_quit shuts the editor down" -Artifact $editorLog -Check {
		try
		{
			Invoke-McpTool -Port $EditorPort -Tool "app_quit" -TimeoutSeconds 60 | Out-Null
		}
		catch
		{
			# the host may close the connection while quitting, which surfaces as a transport error
			Write-Verbose "app_quit did not answer: $($_.Exception.Message)"
		}

		$p = $editorProcess

		if (-not (Wait-ForCondition -TimeoutSeconds 120 -Condition { $p.HasExited }))
		{
			throw "The editor is still running 120s after app_quit."
		}

		if (-not (Wait-ForCondition -TimeoutSeconds 60 -Condition { (@(Get-LeftoverEzProcesses -BinDir $binDir)).Count -eq 0 }))
		{
			$names = (@(Get-LeftoverEzProcesses -BinDir $binDir) | ForEach-Object { $_.ProcessName }) -join ", "
			throw "These processes survived the editor: $names"
		}

		return "no processes left behind"
	}

	# the editor's whole log is only readable once it has exited, so this is the point where it can be
	# written - the 'finally' block below would otherwise be the only place, and it never sees a
	# successful shutdown
	Save-DetachedProcessOutput -Process $editorProcess -LogFile $editorLog

	# only drop the handle once the process is really gone, otherwise the cleanup in 'finally' would
	# have nothing left to kill after a failed shutdown
	if ($editorProcess.HasExited) { $editorProcess = $null }

	# D6 equivalent for the player: the same server, but hosted by the game
	$playerPort = $EditorPort + 10

	Invoke-TestCheck -Name "Player MCP server starts and answers" -Artifact $playerLog -Check {
		if (Test-McpPortOpen -Port $playerPort)
		{
			throw "Port $playerPort is already in use - is another player running?"
		}

		$script:playerProc = Start-EzProcessDetached -Exe $playerExe -WorkingDirectory $SdkDir -Arguments @(
			"-project", $projectDir
			"-scene", $Scene
			"-mcpport", "$playerPort"
		)

		if (-not (Wait-ForCondition -TimeoutSeconds $StartupTimeoutSeconds -Condition { Test-McpPortOpen -Port $playerPort }))
		{
			throw "The player did not open port $playerPort within ${StartupTimeoutSeconds}s."
		}

		$tools = Invoke-McpRequest -Port $playerPort -Method "tools/list" -TimeoutSeconds 300
		$names = @($tools.tools | ForEach-Object { $_.name })
		$missing = @($expectedGameTools | Where-Object { $names -notcontains $_ })

		if ($missing.Count -gt 0)
		{
			throw "These game tools are missing: $($missing -join ', ')"
		}

		# proves that the game loop is actually running, not just the socket
		Invoke-McpTool -Port $playerPort -Tool "game_wait" -Arguments @{ frames = 10 } -TimeoutSeconds 120 | Out-Null

		return "$($names.Count) tools"
	}

	if (Get-Variable -Name playerProc -Scope Script -ErrorAction SilentlyContinue)
	{
		$playerProcess = $script:playerProc
		Remove-Variable -Name playerProc -Scope Script
	}

	if ($null -ne $playerProcess)
	{
		Invoke-TestCheck -Name "app_quit shuts the player down" -Artifact $playerLog -Check {
			try { Invoke-McpTool -Port $playerPort -Tool "app_quit" -TimeoutSeconds 60 | Out-Null } catch { }

			$p = $playerProcess

			if (-not (Wait-ForCondition -TimeoutSeconds 60 -Condition { $p.HasExited }))
			{
				throw "The player is still running 60s after app_quit."
			}

			return "exit code $($p.ExitCode)"
		}

		Save-DetachedProcessOutput -Process $playerProcess -LogFile $playerLog

		if ($playerProcess.HasExited) { $playerProcess = $null }
	}
}
finally
{
	# a failed check must not leave a headless editor running on the machine
	Stop-EzProcessTree -Process $editorProcess
	Stop-EzProcessTree -Process $playerProcess

	# only reached for the processes that were still alive above; the clean shutdown paths write their
	# log right after the corresponding check, because that is where they give the handle up
	Save-DetachedProcessOutput -Process $editorProcess -LogFile $editorLog
	Save-DetachedProcessOutput -Process $playerProcess -LogFile $playerLog

	@(Get-LeftoverEzProcesses -BinDir $binDir) | ForEach-Object {
		Write-Host "Killing leftover process $($_.ProcessName) ($($_.Id))."
		Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
	}
}

exit (Save-TestResults)
