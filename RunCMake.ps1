param
(
    [Parameter(Mandatory = $True)] [ValidateSet('Win64vs2022', 'Uwp64vs2022')][string] $Target,
    [switch]$NoUnityBuild,
    [switch]$NoSubmoduleUpdate,
    [string]$SolutionName,
    [string]$WorkspaceDir
)

Set-Location $PSScriptRoot

if ($NoSubmoduleUpdate -eq $False) {
    $CURRENT_COMMIT = git log -n 1 --format=%H

    Write-Host "Current commit: $CURRENT_COMMIT"

    $UPDATE_SUBMODULES = $True
    $LAST_UPDATE_FILE = "$PSScriptRoot\Data\Content\AssetCache\LastSubmoduleUpdate.txt" 

    if (Test-Path $LAST_UPDATE_FILE -PathType Leaf -ErrorAction SilentlyContinue) {
        $LAST_COMMIT = Get-Content -Path $LAST_UPDATE_FILE

        if ($CURRENT_COMMIT -eq $LAST_COMMIT) {
            Write-Host "Submodules already up-to-date."
            $UPDATE_SUBMODULES = $False
        }
        else {
            Write-Host "Submodules were last updated at commit: $LAST_COMMIT"
        }
    }

    if ($UPDATE_SUBMODULES) {
        Write-Host "Updating submodules"

        git submodule init
        git submodule update

        Out-File ( New-Item -Path $LAST_UPDATE_FILE -Force) -InputObject $CURRENT_COMMIT
    }
}

$CMAKE_ARGS = @("-S", "$PSScriptRoot")

if ($NoUnityBuild) {
    $CMAKE_ARGS += "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF"
}
else {
    $CMAKE_ARGS += "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=ON"
}

if ($SolutionName -ne "") {
    $CMAKE_ARGS += "-DEZ_SOLUTION_NAME:STRING='${SolutionName}'"
}

$CMAKE_ARGS += "-G"

Write-Host ""

$CustomWorkspace = $False

if ($WorkspaceDir -ne "") {
    $CustomWorkspace = $True
}

if ($Target -eq "Win64vs2022") {

    Write-Host "=== Generating Solution for Visual Studio 2022 x64 ==="

    $CMAKE_ARGS += "Visual Studio 17 2022"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"

    if (-not $CustomWorkspace) {
        $WorkspaceDir = "vs2022x64"
    }
}
elseif ($Target -eq "Uwp64vs2022") {

    Write-Host "=== Generating Solution for Visual Studio 2022 x64 UWP ==="

    $CMAKE_ARGS += "Visual Studio 17 2022"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"

    if (-not $CustomWorkspace) {
        $WorkspaceDir = "vs2022x64uwp"
    }

    $CMAKE_ARGS += "-DCMAKE_TOOLCHAIN_FILE=$PSScriptRoot\Code\BuildSystem\CMake\toolchain-winstore.cmake"
    $CMAKE_ARGS += "-DEZ_ENABLE_QT_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_BUILD_FILTER='UwpProjects'"
}
else {
    throw "Unknown target '$Target'."
}

# Add build directory to CMAKE_ARGS
$CMAKE_ARGS += "-B"
$CMAKE_ARGS += "$PSScriptRoot\Workspace\$WorkspaceDir"

Write-Host "Using workspace directory: Workspace\$WorkspaceDir"

# Set custom output directories to avoid conflicts between different workspaces
if ($CustomWorkspace) {
    $CMAKE_ARGS += "-DEZ_OUTPUT_DIRECTORY_DLL:PATH=$PSScriptRoot\Workspace\$WorkspaceDir-output\Bin"
    $CMAKE_ARGS += "-DEZ_OUTPUT_DIRECTORY_LIB:PATH=$PSScriptRoot\Workspace\$WorkspaceDir-output\Lib"
    Write-Host "Custom output directories: Workspace\$WorkspaceDir-output\"
}

Write-Host ""
Write-Host "Running cmake.exe $CMAKE_ARGS"
Write-Host ""
&Data\Tools\Precompiled\cmake\bin\cmake.exe $CMAKE_ARGS

if (!$?) {
    throw "CMake failed with exit code '$LASTEXITCODE'."
}
