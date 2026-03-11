param
(
    [Parameter(Mandatory = $True)] [ValidateSet('vs2022x64', 'vs2026x64', 'android-arm64-debug', 'android-arm64-dev', 'android-arm64-shipping', 'android-x64-debug', 'android-x64-dev', 'android-x64-shipping')][string] $Target,
    [switch]$NoSubmoduleUpdate,
    [switch]$NoUnityBuild,
    [Nullable[bool]]$VulkanSupport,
    [string]$SolutionName,
    [string]$WorkspaceDir
)

Set-Location $PSScriptRoot

#region Helper Functions
function Show-RepositorySetupError {
    param(
        [string]$ErrorReason
    )

    Write-Host ""
    Write-Host "ERROR: $ErrorReason" -ForegroundColor Red
    Write-Host ""
    Write-Host "The ezEngine repository must be cloned using Git." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Please follow these steps:" -ForegroundColor Yellow
    Write-Host "  1. Install Git from: https://git-scm.com/downloads" -ForegroundColor White
    Write-Host "  2. Clone the ezEngine repository using:" -ForegroundColor White
    Write-Host "     git clone https://github.com/ezEngine/ezEngine.git" -ForegroundColor White
    Write-Host ""
    Write-Host "Do NOT download the repository as a ZIP file from GitHub," -ForegroundColor Red
    Write-Host "as this will not include the necessary data." -ForegroundColor Yellow
    Write-Host ""
    throw "Repository setup is invalid. Please clone with Git."
}
#endregion

#region Git Availability Check
Write-Host "Verifying repository setup..." -ForegroundColor Cyan

# Check if git is available
$gitCommand = Get-Command git -ErrorAction SilentlyContinue
if ($null -eq $gitCommand) {
    Show-RepositorySetupError -ErrorReason "Git is not installed or not available in PATH"
}
#endregion

#region Submodule Update
if ($NoSubmoduleUpdate -eq $False) {
    # Verify this is a valid Git repository
    $CURRENT_COMMIT = git log -n 1 --format=%H 2>&1

    if ($LASTEXITCODE -ne 0) {
        Show-RepositorySetupError -ErrorReason "This directory is not a valid Git repository"
    }

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
        Write-Host "Updating submodules..." -ForegroundColor Green

        git submodule init
        git submodule update

        Out-File ( New-Item -Path $LAST_UPDATE_FILE -Force) -InputObject $CURRENT_COMMIT
    }
}
#endregion

#region Repository Sanity Check
# Verify that essential precompiled tools are present (indicates proper git clone with submodules)
$licenseFile = "$PSScriptRoot\Data\Tools\Precompiled\LICENSE.md"
if (-not (Test-Path $licenseFile -PathType Leaf)) {
    Show-RepositorySetupError -ErrorReason "Required submodule files are missing (Data\Tools\Precompiled\LICENSE.md not found)"
}

#endregion

#region Android Dependencies
if ($Target -like "android*") {
    Write-Host "Installing Android Dependencies..." -ForegroundColor Green
    . "$PSScriptRoot/Utilities/Android/InstallAndroidDependencies.ps1"
    . "$PSScriptRoot/Utilities/Android/CompileShaders.ps1"
}
#endregion

#region CMake
$CMAKE_ARGS = @("-S", "$PSScriptRoot", "--preset", "$Target")
if ($NoUnityBuild) {
    $CMAKE_ARGS += "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF"
}
else {
    $CMAKE_ARGS += "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=ON"
}

if ($PSBoundParameters.ContainsKey('VulkanSupport') -and $Target -like "android*" -eq $False) {
    if ($VulkanSupport) {
        $CMAKE_ARGS += "-DEZ_BUILD_EXPERIMENTAL_VULKAN:BOOL=ON"
    }
    else {
        $CMAKE_ARGS += "-DEZ_BUILD_EXPERIMENTAL_VULKAN:BOOL=OFF"
    }
}

if ($SolutionName -ne "") {
    $CMAKE_ARGS += "-DEZ_SOLUTION_NAME:STRING='${SolutionName}'"
}

Write-Host ""

# Set custom output directories to avoid conflicts between different workspaces
if ($WorkspaceDir -ne "") {
    Write-Host "Using custom workspace directory: Workspace\$WorkspaceDir"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += "$PSScriptRoot\Workspace\$WorkspaceDir"
    $CMAKE_ARGS += "-DEZ_OUTPUT_DIRECTORY_DLL:PATH=$PSScriptRoot\Workspace\$WorkspaceDir-output\Bin"
    $CMAKE_ARGS += "-DEZ_OUTPUT_DIRECTORY_LIB:PATH=$PSScriptRoot\Workspace\$WorkspaceDir-output\Lib"
    Write-Host "Custom output directories: Workspace\$WorkspaceDir-output\"
}

Write-Host ""
Write-Host "Running cmake.exe $CMAKE_ARGS" -ForegroundColor Green
Write-Host ""
&Data\Tools\Precompiled\cmake\bin\cmake.exe $CMAKE_ARGS

if (!$?) {
    throw "CMake failed with exit code '$LASTEXITCODE'."
}
#endregion

