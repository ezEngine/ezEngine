<#
.SYNOPSIS
    Set up the workspace for include-cleaner analysis.

.DESCRIPTION
    Prepares everything needed to run RunIncludeCleaner.ps1:

    1. Ensures clang-include-cleaner is available.
       - Windows: Downloads LLVM (same version used by the clang-tidy tooling) into a
         local llvm/ folder if not already present.
       - Linux: Checks for clang-include-cleaner on PATH or under /usr/lib/llvm-*/bin/
         and prints the apt install command if it is missing.

    2. Configures and builds the analysis workspace.
       - Windows: Uses Ninja + Clang from the local llvm/ folder with
         EZ_USE_PCH=OFF, EZ_ENABLE_FOLDER_UNITY_FILES=OFF and
         CMAKE_EXPORT_COMPILE_COMMANDS=ON.
       - Linux: Invokes RunCMake.sh with the linux-clang-include-analysis preset,
         then builds with cmake.

    After this script finishes, RunIncludeCleaner.ps1 can be run directly.

.PARAMETER SkipBuild
    Only configure the workspace without building. Useful when you only want to
    regenerate compile_commands.json after adding new source files.

.EXAMPLE
    # Full setup (configure + build)
    pwsh ./Utilities/include-cleaner/SetupWorkspace.ps1

.EXAMPLE
    # Reconfigure only (skip the build step)
    pwsh ./Utilities/include-cleaner/SetupWorkspace.ps1 -SkipBuild
#>
param(
    [Parameter(Mandatory = $false, HelpMessage = "Only configure, do not build.")]
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$ScriptDir = $PSScriptRoot
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "../..")).Path

$WorkspaceDir = "include-analysis"
$LlvmVersion = "18.1.7"
$LlvmDir = Join-Path $RepoRoot "llvm"

# ── Helper: locate cmake ─────────────────────────────────────────────────────

function Get-CMake {
    if ($IsLinux -or $IsMacOS) {
        $cmd = Get-Command "cmake" -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
        Write-Error "cmake not found. Install it with: sudo apt install cmake"
        exit 1
    }
    # Windows: prefer the precompiled cmake shipped with the repo
    $precompiled = Join-Path $RepoRoot "Data/Tools/Precompiled/cmake/bin/cmake.exe"
    if (Test-Path $precompiled) { return $precompiled }
    $cmd = Get-Command "cmake" -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    Write-Error "cmake not found. Expected at $precompiled"
    exit 1
}

# ── Step 1: Ensure clang-include-cleaner is available ─────────────────────────

Write-Host ""
Write-Host "=== Step 1: Checking for clang-include-cleaner ===" -ForegroundColor Cyan
Write-Host ""

if ($IsLinux -or $IsMacOS) {
    # Look on PATH first, then scan /usr/lib/llvm-*/bin/
    $found = $false
    $tool = Get-Command "clang-include-cleaner" -ErrorAction SilentlyContinue
    if ($tool) {
        Write-Host "Found: $($tool.Source)" -ForegroundColor Green
        $found = $true
    }
    else {
        $candidates = Get-ChildItem "/usr/lib/llvm-*/bin/clang-include-cleaner" -ErrorAction SilentlyContinue |
            Sort-Object { [int]($_.Directory.Parent.Name -replace 'llvm-', '') } -Descending
        if ($candidates) {
            Write-Host "Found: $($candidates[0].FullName)" -ForegroundColor Green
            $found = $true
        }
    }

    if (-not $found) {
        Write-Host "clang-include-cleaner not found." -ForegroundColor Red
        Write-Host ""
        Write-Host "Install it with one of these commands:" -ForegroundColor Yellow
        Write-Host "  sudo apt install clang-tools-20       # Ubuntu 24.04+" -ForegroundColor White
        Write-Host "  sudo apt install clang-tools-18       # Ubuntu 22.04" -ForegroundColor White
        Write-Host ""
        Write-Host "You may need to add the LLVM apt repository first:" -ForegroundColor Yellow
        Write-Host "  See https://apt.llvm.org/ for instructions." -ForegroundColor White
        Write-Host ""
        exit 1
    }
}
else {
    # Windows: check local llvm/ folder
    $includeCleanerExe = Join-Path $LlvmDir "bin/clang-include-cleaner.exe"
    if (Test-Path $includeCleanerExe) {
        Write-Host "Found: $includeCleanerExe" -ForegroundColor Green
    }
    else {
        Write-Host "LLVM not found locally, downloading LLVM $LlvmVersion..." -ForegroundColor Yellow

        $installer = "LLVM-$LlvmVersion-win64.exe"
        $url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$LlvmVersion/$installer"
        $installerPath = Join-Path $RepoRoot $installer

        Write-Host "  Downloading $url ..." -ForegroundColor White
        Invoke-WebRequest $url -OutFile $installerPath

        # Also download Ninja (needed as the CMake generator)
        $ninjaZip = "ninja-win.zip"
        $ninjaUrl = "https://github.com/ninja-build/ninja/releases/download/v1.11.1/$ninjaZip"
        $ninjaZipPath = Join-Path $RepoRoot $ninjaZip

        Write-Host "  Downloading $ninjaUrl ..." -ForegroundColor White
        Invoke-WebRequest $ninjaUrl -OutFile $ninjaZipPath

        # Extract using the precompiled 7z
        $sevenZ = Join-Path $RepoRoot "Data/Tools/Precompiled/7z"
        Write-Host "  Extracting LLVM..." -ForegroundColor White
        & $sevenZ x "-o$LlvmDir" $installerPath | Out-Null
        Write-Host "  Extracting Ninja..." -ForegroundColor White
        & $sevenZ x "-o$LlvmDir" $ninjaZipPath | Out-Null

        # Clean up downloads
        Remove-Item $installerPath -ErrorAction SilentlyContinue
        Remove-Item $ninjaZipPath -ErrorAction SilentlyContinue

        if (-not (Test-Path $includeCleanerExe)) {
            Write-Error "clang-include-cleaner.exe not found after extraction at $includeCleanerExe"
            exit 1
        }

        Write-Host "  LLVM $LlvmVersion installed to $LlvmDir" -ForegroundColor Green
    }
}

# ── Step 2: Configure and build the workspace ────────────────────────────────

Write-Host ""
Write-Host "=== Step 2: Configuring workspace ===" -ForegroundColor Cyan
Write-Host ""

$cmake = Get-CMake

if ($IsLinux -or $IsMacOS) {
    # Use RunCMake.sh with the preset
    $runCMake = Join-Path $RepoRoot "RunCMake.sh"
    Write-Host "Running: $runCMake --target linux-clang-include-analysis --workspace-dir $WorkspaceDir" -ForegroundColor White
    & bash $runCMake --target linux-clang-include-analysis --workspace-dir $WorkspaceDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed."
        exit 1
    }
}
else {
    # Windows: Ninja + Clang from local llvm/ folder
    $WindowsSdkVersions = (Get-ChildItem -Directory "C:\Program Files (x86)\Windows Kits\10\bin").Name |
        Where-Object { $_ -match "^10\.0\." } | Sort-Object -Descending
    if (-not $WindowsSdkVersions) {
        Write-Error "Windows SDK not found under C:\Program Files (x86)\Windows Kits\10\bin"
        exit 1
    }
    $WindowsSdkVersion = $WindowsSdkVersions[0]
    Write-Host "Windows SDK: $WindowsSdkVersion" -ForegroundColor Green

    $rcExe = "C:\Program Files (x86)\Windows Kits\10\bin\$WindowsSdkVersion\x64\rc.exe" -replace '\\', '/'
    $clangCpp = (Join-Path $LlvmDir "bin/clang++.exe") -replace '\\', '/'
    $clangC = (Join-Path $LlvmDir "bin/clang.exe") -replace '\\', '/'
    $ninja = (Join-Path $LlvmDir "ninja.exe") -replace '\\', '/'

    $workspacePath = Join-Path $RepoRoot "Workspace/$WorkspaceDir"
    $outputBin = (Join-Path $RepoRoot "Workspace/$WorkspaceDir-output/Bin") -replace '\\', '/'
    $outputLib = (Join-Path $RepoRoot "Workspace/$WorkspaceDir-output/Lib") -replace '\\', '/'

    $cmakeArgs = @(
        "-G", "Ninja",
        "-S", $RepoRoot,
        "-B", $workspacePath,
        "-DCMAKE_MAKE_PROGRAM=$ninja",
        "-DCMAKE_CXX_COMPILER=$clangCpp",
        "-DCMAKE_C_COMPILER=$clangC",
        "-DCMAKE_RC_COMPILER=$rcExe",
        "-DCMAKE_RC_COMPILER_INIT=rc",
        "-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DEZ_USE_PCH=OFF",
        "-DEZ_ENABLE_FOLDER_UNITY_FILES=OFF",
        "-DEZ_COMPILE_ENGINE_AS_DLL=ON",
        "-DEZ_OUTPUT_DIRECTORY_DLL:PATH=$outputBin",
        "-DEZ_OUTPUT_DIRECTORY_LIB:PATH=$outputLib"
    )

    Write-Host "Running: $cmake $($cmakeArgs -join ' ')" -ForegroundColor White
    & $cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed."
        exit 1
    }
}

# ── Step 3: Build ─────────────────────────────────────────────────────────────

if ($SkipBuild) {
    Write-Host ""
    Write-Host "Configuration complete (-SkipBuild). Skipping build." -ForegroundColor Yellow
    Write-Host "To build manually:" -ForegroundColor Yellow
    if ($IsLinux -or $IsMacOS) {
        Write-Host "  cmake --build Workspace/$WorkspaceDir" -ForegroundColor White
    }
    else {
        Write-Host "  $cmake --build Workspace/$WorkspaceDir" -ForegroundColor White
    }
}
else {
    Write-Host ""
    Write-Host "=== Step 3: Building workspace ===" -ForegroundColor Cyan
    Write-Host ""

    $buildDir = Join-Path $RepoRoot "Workspace/$WorkspaceDir"
    Write-Host "Running: $cmake --build $buildDir" -ForegroundColor White
    & $cmake --build $buildDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed."
        exit 1
    }

    Write-Host ""
    Write-Host "Setup complete." -ForegroundColor Green
    Write-Host "You can now run the include cleaner:" -ForegroundColor Green
    Write-Host "  pwsh ./Utilities/include-cleaner/RunIncludeCleaner.ps1 -Library Foundation" -ForegroundColor White
}
