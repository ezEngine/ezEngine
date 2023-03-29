param 
(
    [Parameter(Mandatory = $True)] [ValidateSet('Win64vs2019', 'Uwp64vs2019', 'Win64vs2022', 'Uwp64vs2022')][string] $Target,
    [switch]$NoUnityBuild,
    [switch]$NoSubmoduleUpdate
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

$CMAKE_ARGS += "-G"

Write-Host ""

if ($Target -eq "Win64vs2019") {

    Write-Host "=== Generating Solution for Visual Studio 2019 x64 ==="

    $CMAKE_ARGS += "Visual Studio 16 2019"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += "$PSScriptRoot\Workspace\vs2019x64"
}
elseif ($Target -eq "Uwp64vs2019") {

    Write-Host "=== Generating Solution for Visual Studio 2019 x64 UWP ==="

    $CMAKE_ARGS += "Visual Studio 16 2019"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += "$PSScriptRoot\Workspace\vs2019x64uwp"
    $CMAKE_ARGS += "-DCMAKE_TOOLCHAIN_FILE=$PSScriptRoot\Code\BuildSystem\CMake\toolchain-winstore.cmake"
}
elseif ($Target -eq "Win64vs2022") {

    Write-Host "=== Generating Solution for Visual Studio 2022 x64 ==="

    $CMAKE_ARGS += "Visual Studio 17 2022"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += "$PSScriptRoot\Workspace\vs2022x64"
}
elseif ($Target -eq "Uwp64vs2022") {

    Write-Host "=== Generating Solution for Visual Studio 2022 x64 UWP ==="

    $CMAKE_ARGS += "Visual Studio 17 2022"
    $CMAKE_ARGS += "-A"
    $CMAKE_ARGS += "x64"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += "$PSScriptRoot\Workspace\vs2022x64uwp"
    $CMAKE_ARGS += "-DCMAKE_TOOLCHAIN_FILE=$PSScriptRoot\Code\BuildSystem\CMake\toolchain-winstore.cmake"
}
else {
    throw "Unknown target '$Target'."
}

Write-Host ""
Write-Host "Running cmake.exe $CMAKE_ARGS"
Write-Host ""
&Data\Tools\Precompiled\cmake\bin\cmake.exe $CMAKE_ARGS

if (!$?) {
    throw "CMake failed with exit code '$LASTEXITCODE'."
}
