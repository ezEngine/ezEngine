<#
.SYNOPSIS
    Find and optionally remove unnecessary #include directives using clang-include-cleaner.

.DESCRIPTION
    Analyzes C++ source files for unnecessary #include directives using clang-include-cleaner
    (LLVM 17+). Operates against a compile_commands.json built with EZ_USE_PCH=OFF and
    EZ_ENABLE_FOLDER_UNITY_FILES=OFF so that each translation unit is compiled individually.

    The tool only suggests removals. Insertion suggestions from clang-include-cleaner are
    filtered out, as are suggestions to remove PCH headers (kept by convention).

    Prerequisites:
      1. clang-include-cleaner installed and on PATH, or discoverable under
         /usr/lib/llvm-*/bin/ (Linux) or in a local llvm/ folder (Windows).
      2. A configured and fully built workspace with compile_commands.json.
         On Linux, use the linux-clang-include-analysis CMake preset:
           ./RunCMake.sh --target linux-clang-include-analysis --workspace-dir include-analysis
           cmake --build Workspace/include-analysis
      3. The target library must be built before analysis so all generated headers exist.

.PARAMETER Library
    Name of the library to analyze (e.g., Foundation, Core, RendererCore, JoltPlugin).
    The script searches for a matching directory under Code/Engine/, Code/EnginePlugins/,
    Code/Editor/, Code/EditorPlugins/, and Code/Tools/.
    Either -Library or -File must be specified.

.PARAMETER File
    Path to a single source file to analyze, relative to the repository root
    (e.g., Code/Engine/Foundation/Logging/Implementation/Log.cpp).
    Either -Library or -File must be specified.

.PARAMETER CompileDb
    Path to compile_commands.json. Defaults to Workspace/include-analysis/compile_commands.json.

.PARAMETER Apply
    When set, removes the identified unnecessary #include lines from the source files.
    Without this flag, the script only reports what it would remove (dry-run).

.PARAMETER IgnoreHeaders
    Comma-separated list of regexes matched against header suffixes. Headers matching
    any regex are excluded from analysis. Passed directly to clang-include-cleaner's
    --ignore-headers flag. System/compiler internals (__*, bits/*, ext/*) are always ignored.

.PARAMETER IncludeCleaner
    Explicit path to the clang-include-cleaner binary. If not set, the script searches
    PATH, /usr/lib/llvm-*/bin/ (Linux), a local llvm/ folder, and common install
    locations (Windows).

.EXAMPLE
    # Dry-run: show unnecessary includes in Foundation
    pwsh ./Utilities/include-cleaner/RunIncludeCleaner.ps1 -Library Foundation

.EXAMPLE
    # Apply removals to Foundation
    pwsh ./Utilities/include-cleaner/RunIncludeCleaner.ps1 -Library Foundation -Apply

.EXAMPLE
    # Single file analysis
    pwsh ./Utilities/include-cleaner/RunIncludeCleaner.ps1 -File Code/Engine/Foundation/Logging/Implementation/Log.cpp

.EXAMPLE
    # Analyze Core with custom ignore patterns
    pwsh ./Utilities/include-cleaner/RunIncludeCleaner.ps1 -Library Core -IgnoreHeaders "RendererVulkan/.*"
#>
param(
    [Parameter(Mandatory = $false, HelpMessage = "Library name to analyze (e.g., Foundation, Core).")]
    [string]$Library,

    [Parameter(Mandatory = $false, HelpMessage = "Single source file to analyze (repo-relative path).")]
    [string]$File,

    [Parameter(Mandatory = $false, HelpMessage = "Path to compile_commands.json.")]
    [string]$CompileDb,

    [Parameter(Mandatory = $false, HelpMessage = "Apply removals to source files instead of dry-run.")]
    [switch]$Apply,

    [Parameter(Mandatory = $false, HelpMessage = "Comma-separated regexes of headers to ignore.")]
    [string]$IgnoreHeaders,

    [Parameter(Mandatory = $false, HelpMessage = "Explicit path to the clang-include-cleaner binary.")]
    [string]$IncludeCleaner
)

$ErrorActionPreference = "Stop"

$ScriptDir = $PSScriptRoot
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "../..")).Path

$Jobs = [Math]::Max(1, [Environment]::ProcessorCount - 1)

# ── Locate compile_commands.json ──────────────────────────────────────────────

if (-not $CompileDb) {
    $CompileDb = Join-Path $RepoRoot "Workspace/include-analysis/compile_commands.json"
}

if (-not (Test-Path $CompileDb)) {
    Write-Error @"
compile_commands.json not found at $CompileDb.
Run:
  Linux:   ./RunCMake.sh --target linux-clang-include-analysis --workspace-dir include-analysis
           cmake --build Workspace/include-analysis
  Windows: Configure a build with EZ_USE_PCH=OFF, EZ_ENABLE_FOLDER_UNITY_FILES=OFF,
           CMAKE_EXPORT_COMPILE_COMMANDS=ON and build it.
"@
    exit 1
}

$CompileDb = (Resolve-Path $CompileDb).Path

# ── Locate clang-include-cleaner ──────────────────────────────────────────────

function Find-IncludeCleaner {
    if ($IncludeCleaner -and (Test-Path $IncludeCleaner)) {
        return $IncludeCleaner
    }

    # Check PATH
    $onPath = Get-Command "clang-include-cleaner" -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }

    if ($IsLinux -or $IsMacOS) {
        # Search /usr/lib/llvm-*/bin/
        $candidates = Get-ChildItem "/usr/lib/llvm-*/bin/clang-include-cleaner" -ErrorAction SilentlyContinue |
            Sort-Object { [int]($_.Directory.Parent.Name -replace 'llvm-', '') } -Descending
        if ($candidates) { return $candidates[0].FullName }
    }
    else {
        # Windows: check local llvm/ folder and common install paths
        $candidates = @(
            (Join-Path $RepoRoot "llvm/bin/clang-include-cleaner.exe"),
            "C:\Program Files\LLVM\bin\clang-include-cleaner.exe"
        )
        foreach ($c in $candidates) {
            if (Test-Path $c) { return $c }
        }
    }

    Write-Error @"
clang-include-cleaner not found.
  Linux:   sudo apt install clang-tools-<version>  (e.g., clang-tools-20)
  Windows: Download LLVM from https://github.com/llvm/llvm-project/releases
"@
    exit 1
}

$IncludeCleanerBin = Find-IncludeCleaner
Write-Host "Tool: $IncludeCleanerBin" -ForegroundColor Cyan

# ── Validate parameters ──────────────────────────────────────────────────────

if (-not $Library -and -not $File) {
    Write-Error "Either -Library <name> or -File <path> must be specified."
    exit 1
}

# ── Resolve library path ─────────────────────────────────────────────────────

function Resolve-LibraryPath {
    param([string]$Name)
    $searchPaths = @(
        "Code/Engine/$Name",
        "Code/EnginePlugins/$Name",
        "Code/Editor/$Name",
        "Code/EditorPlugins/$Name",
        "Code/Tools/$Name"
    )
    foreach ($p in $searchPaths) {
        $full = Join-Path $RepoRoot $p
        if (Test-Path $full -PathType Container) { return $p }
    }
    Write-Error "Cannot find library '$Name' under Code/Engine/, Code/EnginePlugins/, Code/Editor/, Code/EditorPlugins/, or Code/Tools/"
    exit 1
}

# ── Filter compile_commands.json ──────────────────────────────────────────────

function Read-CompileDb {
    param([string]$Path)
    # Parse compile_commands.json manually (no jq dependency)
    $content = Get-Content $Path -Raw
    return ($content | ConvertFrom-Json)
}

function Filter-CompileDb {
    param(
        [object[]]$Entries,
        [string]$FilterPath
    )
    # Normalize path separators for matching
    $filterNormalized = $FilterPath -replace '\\', '/'

    $filtered = $Entries | Where-Object {
        $file = $_.file -replace '\\', '/'
        ($file -match [regex]::Escape($filterNormalized)) -and
        ($file -notmatch 'ThirdParty|moc_|qrc_|\.rc$') -and
        ($file -notmatch 'PCH\.cpp$')
    }

    return @($filtered)
}

function Write-CompileDb {
    param(
        [object[]]$Entries,
        [string]$OutputPath
    )
    # ConvertTo-Json doesn't wrap single items in an array, but compile_commands.json requires one.
    # Force array notation by wrapping in @() and using -AsArray (PowerShell 7+).
    @($Entries) | ConvertTo-Json -Depth 10 -AsArray | Set-Content $OutputPath -Encoding UTF8
}

# ── Parse clang-include-cleaner output ────────────────────────────────────────

function Parse-Removals {
    param(
        [string]$Output,
        [string]$FilePath
    )
    # Each line like: - <Foundation/Application/Application.h> @Line:3
    $removals = @()
    foreach ($line in ($Output -split "`n")) {
        $line = $line.Trim()
        # Skip insertion suggestions (+ lines) and PCH includes
        if ($line -match '^-\s+(.+)\s+@Line:(\d+)$') {
            $header = $Matches[1]
            $lineNum = [int]$Matches[2]
            # Skip PCH headers
            if ($header -match 'PCH\.h') { continue }
            $removals += [PSCustomObject]@{
                Header = $header
                Line   = $lineNum
            }
        }
    }
    return $removals
}

# ── Apply removals to a file ─────────────────────────────────────────────────

function Apply-Removals {
    param(
        [string]$FilePath,
        [int[]]$LineNumbers
    )
    if (-not (Test-Path $FilePath)) {
        Write-Warning "File not found: $FilePath"
        return 0
    }

    $lines = Get-Content $FilePath
    $linesToRemove = [System.Collections.Generic.HashSet[int]]::new()
    $removed = 0

    foreach ($ln in $LineNumbers) {
        if ($ln -le $lines.Count) {
            $content = $lines[$ln - 1].Trim()
            if ($content -match '^#include') {
                [void]$linesToRemove.Add($ln)
            }
            else {
                Write-Warning "${FilePath}:${ln} is not an #include ('$content'), skipping"
            }
        }
        else {
            Write-Warning "${FilePath}:${ln} is beyond end of file ($($lines.Count) lines)"
        }
    }

    if ($linesToRemove.Count -eq 0) { return 0 }

    $newLines = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if (-not $linesToRemove.Contains($i + 1)) {
            $newLines += $lines[$i]
        }
    }

    Set-Content $FilePath -Value $newLines -Encoding UTF8
    return $linesToRemove.Count
}

# ── Process a single source file ─────────────────────────────────────────────

function Process-SourceFile {
    param(
        [string]$SourceFile,
        [string]$DbDir,
        [string]$ExtraArgs
    )

    $relFile = $SourceFile
    if ($SourceFile.StartsWith($RepoRoot)) {
        $relFile = $SourceFile.Substring($RepoRoot.Length).TrimStart('/', '\')
    }

    $args2 = @("-p", $DbDir, "--print=changes")
    if ($ExtraArgs) {
        $args2 += ($ExtraArgs -split '\s+' | Where-Object { $_ })
    }
    $args2 += $SourceFile

    $output = & $IncludeCleanerBin @args 2>&1 | Out-String
    $removals = Parse-Removals -Output $output -FilePath $SourceFile

    return [PSCustomObject]@{
        RelPath  = $relFile
        AbsPath  = $SourceFile
        Removals = $removals
    }
}

# ── Main ──────────────────────────────────────────────────────────────────────

# Determine the filter path
if ($File) {
    $filterPath = $File -replace '\\', '/'
    $absFile = Join-Path $RepoRoot $File
    if (-not (Test-Path $absFile)) {
        Write-Error "File not found: $absFile"
        exit 1
    }
    Write-Host "Single file: $File"
}
else {
    $libPath = Resolve-LibraryPath $Library
    $filterPath = $libPath
    Write-Host "Library: $Library -> $libPath"
}

# Read and filter compile_commands.json
Write-Host "Reading compile_commands.json..." -ForegroundColor Gray
$allEntries = Read-CompileDb $CompileDb
$filtered = Filter-CompileDb $allEntries $filterPath

if ($filtered.Count -eq 0) {
    Write-Error "No files matched filter '$filterPath'. Check library name and compile_commands.json."
    exit 1
}

Write-Host "Found $($filtered.Count) compilation units to analyze." -ForegroundColor Green

# Write filtered DB to a temp directory
$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "ez-include-cleaner-$(Get-Random)"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

try {
    $filteredDbPath = Join-Path $tempDir "compile_commands.json"
    Write-CompileDb $filtered $filteredDbPath

    # Build extra args string
    $extraArgs = ""
    if ($IgnoreHeaders) {
        $extraArgs += "--ignore-headers=$IgnoreHeaders "
    }
    $extraArgs += "--ignore-headers=__.*,bits/.*,ext/.*"

    $mode = if ($Apply) { "APPLY" } else { "DRY-RUN" }
    Write-Host "Mode: $mode" -ForegroundColor $(if ($Apply) { "Red" } else { "Yellow" })
    Write-Host ("=" * 64) -ForegroundColor DarkGray

    # Extract file list
    $sourceFiles = $filtered | ForEach-Object { $_.file }

    # Process files in parallel using ForEach-Object -Parallel (PowerShell 7+)
    $results = $sourceFiles | ForEach-Object -ThrottleLimit $Jobs -Parallel {
        $SourceFile = $_
        $IncludeCleanerBin = $using:IncludeCleanerBin
        $tempDir = $using:tempDir
        $extraArgs = $using:extraArgs
        $RepoRoot = $using:RepoRoot

        $relFile = $SourceFile
        if ($SourceFile.StartsWith($RepoRoot)) {
            $relFile = $SourceFile.Substring($RepoRoot.Length).TrimStart('/', '\')
        }

        $toolArgs = @("-p", $tempDir, "--print=changes")
        if ($extraArgs) {
            $toolArgs += ($extraArgs -split '\s+' | Where-Object { $_ })
        }

        $toolArgs += $SourceFile

        $output = & $IncludeCleanerBin @toolArgs 2>&1 | Out-String

        # Parse removals
        $removals = @()
        foreach ($line in ($output -split "`n")) {
            $cleanLine = $line.Trim()
            if ($cleanLine -match '^-\s+(.+)\s+@Line:(\d+)$') {
                $header = $Matches[1]
                $lineNum = [int]$Matches[2]
                if ($header -notmatch 'PCH\.h') {
                    $removals += [PSCustomObject]@{
                        Header = $header
                        Line   = $lineNum
                    }
                }
            }
        }

        if ($removals.Count -gt 0) {
            [PSCustomObject]@{
                RelPath  = $relFile
                AbsPath  = $SourceFile
                Removals = $removals
            }
        }
    }

    # Ensure results is always an array
    $results = @($results | Where-Object { $null -ne $_ })

    Write-Host ("=" * 64) -ForegroundColor DarkGray
    Write-Host "Analysis complete: $($results.Count) / $($sourceFiles.Count) files have suggested removals." -ForegroundColor Green

    # Display results
    $totalRemovals = 0
    foreach ($r in $results) {
        Write-Host "`n── $($r.RelPath) ──" -ForegroundColor White
        foreach ($removal in $r.Removals) {
            Write-Host "- $($removal.Header) @Line:$($removal.Line)" -ForegroundColor Red
            $totalRemovals++
        }
    }

    Write-Host "`nTotal: $totalRemovals unnecessary include(s) in $($results.Count) file(s)." -ForegroundColor Cyan

    # Apply removals if requested
    if ($Apply -and $results.Count -gt 0) {
        Write-Host "`nApplying removals..." -ForegroundColor Yellow
        $totalApplied = 0
        foreach ($r in $results) {
            $lineNums = @($r.Removals | ForEach-Object { $_.Line })
            $applied = Apply-Removals -FilePath $r.AbsPath -LineNumbers $lineNums
            if ($applied -gt 0) {
                Write-Host "  $($r.RelPath): removed $applied include(s)" -ForegroundColor Green
                $totalApplied += $applied
            }
        }
        Write-Host "`nApplied: removed $totalApplied include(s) from $($results.Count) file(s)." -ForegroundColor Green
        Write-Host "Verify with: cmake --build Workspace/include-analysis" -ForegroundColor Yellow
    }

}
finally {
    # Clean up temp directory
    if (Test-Path $tempDir) {
        Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
