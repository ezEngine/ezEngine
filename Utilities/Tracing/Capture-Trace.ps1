#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Cross-platform trace capture for ezEngine.

.DESCRIPTION
    Captures OS-level trace events from ezEngine applications.

    Without -Start or -Stop, the script runs interactively: it starts
    recording, waits for a keypress, then stops and saves the trace.

    With -Start, the script begins recording and returns immediately.
    With -Stop, the script stops a previously started session and saves the trace.
    These modes allow automation (e.g. from a unit test).

    The platform is detected automatically. Use -Android to target a connected
    Android device via adb/Perfetto.

    ── Prerequisites ──

    Windows (ETW via Windows Performance Recorder):
      - wpr.exe must be on PATH (ships with Windows 10+).
      - Must run as Administrator.
      - The provided profile (ezTraceProvider.wprp) is used automatically.

    Linux (LTTNG user-space tracing):
      - Install LTTNG tools:
          sudo apt install lttng-tools lttng-modules-dkms liblttng-ust-dev
      - Build ezEngine with EZ_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT=ON (default for Linux).
      - Make sure your user is in the 'tracing' group:
          sudo usermod -aG tracing $USER
          (log out and back in for the group change to take effect)
      - Install babeltrace2 for text dumps:
          sudo apt install babeltrace2

    Android (Perfetto via adb):
      - Build ezEngine with EZ_3RDPARTY_PERFETTO_SUPPORT=ON (default for Android).
      - 'adb' must be on PATH with a device running Android 9+.
      - The Perfetto config (ez-perfetto.pbtx) is used automatically.

    ── Platform Details ──

    Windows — ETW
      This script drives wpr.exe (Windows Performance Recorder) with the
      ezTraceProvider.wprp profile.

      To capture manually without this script:
        wpr -start Utilities\Tracing\ezTraceProvider.wprp -instancename ezEngine
        # ... run your application ...
        wpr -stop MyTrace.etl -instancename ezEngine
        wpa MyTrace.etl

    Linux — LTTNG
      Uses the tracelogging-lttng wrapper (Code/ThirdParty/tracelogging) which
      provides the same TraceLoggingWrite API as Windows ETW, backed by LTTNG-UST.
      Each library registers its own provider (e.g. "ez_Foundation").
      The wildcard 'ez_*:*' captures all of them. Process/thread context
      (vpid, vtid, procname) is added automatically so Trace Compass can
      identify threads.

      To capture manually without this script:
        lttng create ez-session
        lttng enable-event -u 'ez_*:*'
        lttng add-context -u -t vpid -t vtid -t procname
        lttng start
        # ... run your application ...
        lttng stop
        lttng destroy
        babeltrace2 ~/lttng-traces/ez-session-*
      Or open in Trace Compass: https://www.eclipse.org/tracecompass/

      Note: To add kernel events (requires root):
        sudo lttng enable-event -k sched_switch,sched_wakeup

    Android — Perfetto
      The Perfetto SDK connects to the system 'traced' daemon (Android 9+).
      A ready-to-use trace config is provided at Utilities/Tracing/ez-perfetto.pbtx.

      To capture manually without this script:
        adb push Utilities/Tracing/ez-perfetto.pbtx /data/local/tmp/
        adb shell perfetto -c /data/local/tmp/ez-perfetto.pbtx \
          -o /data/misc/perfetto-traces/ez.perfetto-trace
        adb pull /data/misc/perfetto-traces/ez.perfetto-trace .
      Open in https://ui.perfetto.dev. Events appear under the "ez" category.

.PARAMETER Start
    Start recording and return immediately. Mutually exclusive with -Stop.

.PARAMETER Stop
    Stop a previously started recording and save the trace. Mutually exclusive with -Start.

.PARAMETER Android
    Capture a Perfetto trace on a connected Android device via adb.
    Requires 'adb' on the host and a device running Android 9+.

.PARAMETER Cleanup
    (Android only) Restart the system 'traced' daemon on the device to clear
    stale producer registrations that can cause empty traces. Requires the
    device to allow 'adb root'. This is a standalone action and does not
    start or stop a trace.

.PARAMETER OutputPath
    Path for the trace output. If not specified, the trace is saved in the
    current working directory as ez-trace-YYYY-MM-DDTHH-MM-SS with a
    platform-appropriate extension (.etl on Windows, .perfetto-trace on
    Android) or as a directory (Linux/LTTNG).

.EXAMPLE
    # Interactive capture on Windows or Linux (auto-detected):
    ./Capture-Trace.ps1

.EXAMPLE
    # Non-interactive (for automation / unit tests):
    ./Capture-Trace.ps1 -Start
    # ... run application ...
    ./Capture-Trace.ps1 -Stop -OutputPath trace.etl

.EXAMPLE
    # Capture a Perfetto trace from a connected Android device:
    ./Capture-Trace.ps1 -Android

.EXAMPLE
    # Clear stale Perfetto state on the Android device:
    ./Capture-Trace.ps1 -Android -Cleanup
#>

[CmdletBinding()]
param(
    [switch]$Start,
    [switch]$Stop,
    [switch]$Android,
    [switch]$Cleanup,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

if ($Start -and $Stop) {
    throw 'The -Start and -Stop switches are mutually exclusive.'
}
if ($Cleanup -and ($Start -or $Stop)) {
    throw 'The -Cleanup switch cannot be combined with -Start or -Stop.'
}
if ($Cleanup -and -not $Android) {
    throw 'The -Cleanup switch is only supported with -Android.'
}

# Import Android helpers when targeting an Android device.
if ($Android) {
    $RepoRoot = (Resolve-Path (Join-Path $ScriptDir '../..')).Path
    . (Join-Path $RepoRoot 'Utilities/Android/AndroidUtils.ps1')
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function Wait-ForKeypress {
    Write-Host ''
    Write-Host '  Recording... Press any key to stop.' -ForegroundColor Green
    Write-Host ''
    try {
        $null = [Console]::ReadKey($true)
    } catch {
        # Fallback when stdin is redirected or ReadKey is unavailable.
        Read-Host '  Press Enter to stop'
    }
}

function Assert-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' not found. Please install it and ensure it is on your PATH."
    }
}

function Invoke-Adb {
    # Convenience wrapper: calls adb via Get-Adb without automatic error
    # checking. Use this for calls where the caller inspects $LASTEXITCODE
    # or the output directly (e.g. 'adb devices', 'adb pull' with fallback).
    param([Parameter(ValueFromRemainingArguments)][string[]]$Arguments)
    return & (Get-Adb) @Arguments
}

function Get-DefaultOutputPath {
    param([string]$Extension)
    $timestamp = (Get-Date).ToString('yyyy-MM-ddTHH-mm-ss')
    return Join-Path (Get-Location) "ez-trace-$timestamp$Extension"
}

# ---------------------------------------------------------------------------
# Windows — ETW via Windows Performance Recorder
# ---------------------------------------------------------------------------

function Invoke-WindowsTraceBegin {
    Assert-Command 'wpr'

    $wprpPath = Join-Path $ScriptDir 'ezTraceProvider.wprp'
    if (-not (Test-Path $wprpPath)) {
        throw "WPR profile not found: $wprpPath"
    }

    # Cancel any stale session from a previous interrupted run.
    & wpr -cancel -instancename ezEngine 2>&1 | Out-Null

    Write-Host 'Starting ETW trace...' -ForegroundColor Cyan
    & wpr -start $wprpPath -instancename ezEngine
    if ($LASTEXITCODE -ne 0) { throw "wpr -start failed (exit code $LASTEXITCODE)." }
}

function Invoke-WindowsTraceEnd {
    $output = if ($OutputPath) { $OutputPath } else { Get-DefaultOutputPath '.etl' }

    Write-Host 'Stopping ETW trace...' -ForegroundColor Cyan
    & wpr -stop $output -instancename ezEngine
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "wpr -stop failed (exit code $LASTEXITCODE). You may need to run 'wpr -cancel -instancename ezEngine' manually."
        return
    }

    Write-Host ''
    Write-Host "Trace saved to: $output" -ForegroundColor Yellow
    Write-Host "Open with:      wpa $output"
}

# ---------------------------------------------------------------------------
# Linux — LTTNG
# ---------------------------------------------------------------------------

$script:LttngSessionName = 'ez-session'

function Invoke-LinuxTraceBegin {
    Assert-Command 'lttng'

    # Destroy any stale session from a previous interrupted run.
    & lttng destroy $script:LttngSessionName 2>&1 | Out-Null

    Write-Host "Creating LTTNG session '$script:LttngSessionName'..." -ForegroundColor Cyan
    & lttng create $script:LttngSessionName
    if ($LASTEXITCODE -ne 0) { throw 'lttng create failed.' }

    & lttng enable-event -u 'ez_*:*'
    if ($LASTEXITCODE -ne 0) { throw 'lttng enable-event failed.' }

    # Attach process/thread context so Trace Compass can identify threads.
    & lttng add-context -u -t vpid -t vtid -t procname
    if ($LASTEXITCODE -ne 0) { throw 'lttng add-context failed.' }

    & lttng start
    if ($LASTEXITCODE -ne 0) { throw 'lttng start failed.' }
}

function Invoke-LinuxTraceEnd {
    Write-Host 'Stopping LTTNG trace...' -ForegroundColor Cyan
    & lttng stop 2>&1 | Out-Null

    # Extract the trace path before destroying the session.
    # Older lttng versions say "Trace path:", newer ones say "Trace output:".
    $traceInfo = & lttng list $script:LttngSessionName 2>&1 | Select-String 'Trace (path|output):'
    $tracePath = if ($traceInfo) { ($traceInfo[0].Line -replace '.*Trace (path|output):\s*', '').Trim() } else { $null }

    & lttng destroy $script:LttngSessionName 2>&1 | Out-Null

    if ($tracePath) {
        $output = if ($OutputPath) { $OutputPath } else { Get-DefaultOutputPath '' }
        # Remove stale trace data from previous runs so old channel files don't
        # mix with the new ones and confuse babeltrace2.
        if (Test-Path $output) { Remove-Item -Recurse -Force $output }
        Copy-Item -Recurse -Force $tracePath $output
        $tracePath = $output
    }

    Write-Host ''
    if ($tracePath) {
        Write-Host "Trace saved to: $tracePath" -ForegroundColor Yellow
        Write-Host "View with:      babeltrace2 $tracePath"
        Write-Host 'Or open in TraceCompass: https://www.eclipse.org/tracecompass/'
    }
}

# ---------------------------------------------------------------------------
# Android — Perfetto via adb
# ---------------------------------------------------------------------------

$script:PerfettoDeviceTrace  = '/data/misc/perfetto-traces/ez.perfetto-trace'
$script:PerfettoDeviceConfig = '/data/local/tmp/ez-perfetto-interactive.pbtx'
$script:PerfettoPidFile      = '/data/local/tmp/ez-perfetto.pid'

function Invoke-AndroidTraceBegin {
    $configPath = Join-Path $ScriptDir 'ez-perfetto.pbtx'
    if (-not (Test-Path $configPath)) {
        throw "Perfetto config not found: $configPath"
    }

    # Verify a device is connected.
    $devices = (Invoke-Adb devices) -join "`n"
    if ($devices -notmatch '\t(device|emulator)') {
        throw "No Android device connected. Check 'adb devices'."
    }

    # Push the config to the device.
    Write-Host 'Preparing config...' -ForegroundColor Cyan
    Adb-Cmd push $configPath $script:PerfettoDeviceConfig | Out-Null

    # Kill any leftover perfetto process from a previous interrupted run.
    $oldPid = Adb-Shell "cat $($script:PerfettoPidFile) 2>/dev/null || true"
    if ($oldPid -match '^\d+$') {
        Adb-Shell "kill $oldPid 2>/dev/null || true" | Out-Null
    }

    Write-Host 'Starting Perfetto trace...' -ForegroundColor Cyan
    # Use -d (background) and --txt (text-format config). --detach/--attach
    # are not available on older Perfetto versions (e.g. v15 on Android 10/11).
    $perfOutput = Adb-Shell "perfetto --txt -d -c $($script:PerfettoDeviceConfig) -o $($script:PerfettoDeviceTrace)"

    # Store the PID so -Stop can find and kill it later.
    $pidValue = ($perfOutput | Select-String '^\d+').Matches.Value
    if ($pidValue) {
        Adb-Shell "echo $pidValue > $($script:PerfettoPidFile)" | Out-Null
        Write-Host "Perfetto running in background (PID $pidValue)." -ForegroundColor Green
    } else {
        Write-Warning "Could not determine perfetto PID. You may need to stop it manually."
    }
}

function Invoke-AndroidTraceEnd {
    $output = if ($OutputPath) { $OutputPath } else { Get-DefaultOutputPath '.perfetto-trace' }

    Write-Host 'Stopping Perfetto trace...' -ForegroundColor Cyan
    # Send SIGINT to perfetto so it flushes and writes the trace cleanly.
    $perfPid = Adb-Shell "cat $($script:PerfettoPidFile) 2>/dev/null || true"
    if ($perfPid -match '^\d+$') {
        Adb-Shell "kill -INT $perfPid" | Out-Null
        Adb-Shell "rm -f $($script:PerfettoPidFile)" | Out-Null
    } else {
        Write-Warning 'No stored perfetto PID found. The trace may have already stopped.'
    }

    # Give perfetto a moment to flush the final buffers.
    Start-Sleep -Milliseconds 1000

    Write-Host 'Pulling trace from device...' -ForegroundColor Cyan
    # The trace file under /data/misc/perfetto-traces/ is owned by shell with
    # mode 600. adb pull cannot read it directly, so copy it to /data/local/tmp/
    # first where adb pull has access.
    $tmpTrace = '/data/local/tmp/ez-pulled.perfetto-trace'
    Adb-Shell "cp $($script:PerfettoDeviceTrace) $tmpTrace && chmod 644 $tmpTrace" | Out-Null
    Invoke-Adb pull $tmpTrace $output
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "adb pull failed. The trace may still be on the device at $($script:PerfettoDeviceTrace)."
    } else {
        Adb-Shell "rm -f $tmpTrace" | Out-Null
        Write-Host ''
        Write-Host "Trace saved to: $output" -ForegroundColor Yellow
        Write-Host "Open in:        https://ui.perfetto.dev"
    }
}

function Invoke-AndroidCleanup {
    Write-Host 'Restarting the Perfetto traced daemon to clear stale state...' -ForegroundColor Cyan
    Write-Host '(This requires the device to allow adb root.)' -ForegroundColor DarkGray

    $adb = Get-Adb

    # Switch to root so we can restart system services.
    $rootResult = & $adb root 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "'adb root' failed. The device may not be rooted or may not allow root access via adb.`nOutput: $rootResult"
    }
    Start-Sleep -Seconds 3

    # Reconnect if using a network device (adb root restarts adbd).
    $devices = (& $adb devices) -join "`n"
    if ($devices -notmatch '\t(device|emulator)') {
        # Try to reconnect to the same device.
        & $adb reconnect 2>&1 | Out-Null
        Start-Sleep -Seconds 2
    }

    # Restart traced and traced_probes to clear all stale producer registrations.
    & $adb shell 'stop traced_probes && stop traced && start traced && start traced_probes' 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        & $adb unroot 2>&1 | Out-Null
        throw 'Failed to restart traced daemon.'
    }
    Start-Sleep -Seconds 1

    # Drop root privileges.
    & $adb unroot 2>&1 | Out-Null
    Start-Sleep -Seconds 3

    # Verify connection is still alive after unroot.
    $devices = (& $adb devices) -join "`n"
    if ($devices -notmatch '\t(device|emulator)') {
        & $adb reconnect 2>&1 | Out-Null
        Start-Sleep -Seconds 2
    }

    Write-Host 'Perfetto traced daemon restarted. Stale producers cleared.' -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Main — dispatch to the right platform and mode
# ---------------------------------------------------------------------------

# Determine which platform functions to call.
if ($Android) {
    $beginFunc = 'Invoke-AndroidTraceBegin'
    $endFunc   = 'Invoke-AndroidTraceEnd'
} elseif ($IsWindows) {
    $beginFunc = 'Invoke-WindowsTraceBegin'
    $endFunc   = 'Invoke-WindowsTraceEnd'
} elseif ($IsLinux) {
    $beginFunc = 'Invoke-LinuxTraceBegin'
    $endFunc   = 'Invoke-LinuxTraceEnd'
} else {
    Write-Error 'OS-level tracing is not supported on this platform. Use -Android for Android device tracing.'
    exit 1
}

if ($Cleanup) {
    Invoke-AndroidCleanup
} elseif ($Start) {
    # Non-interactive: start recording and return.
    & $beginFunc
} elseif ($Stop) {
    # Non-interactive: stop a previously started recording.
    & $endFunc
} else {
    # Interactive: start, wait for keypress, stop.
    $started = $false
    try {
        & $beginFunc
        $started = $true
        Wait-ForKeypress
    } finally {
        if ($started) { & $endFunc }
    }
}
