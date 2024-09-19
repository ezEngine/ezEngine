#!/usr/bin/env powershell

param 
(
    [switch]$RerunCMake,
    [switch]$Clean,
    [ValidateSet('Debug', 'Dev', 'Shipping')][string] $BuildType = "Debug"
)

$EMSCRIPTEN_VERSION = "3.1.65"
$SDK_PATH = "$PSScriptRoot/../../.."
$WORKSPACE_PATH = "$SDK_PATH/Workspace"
$EMSDK_PATH = "$WORKSPACE_PATH/emsdk"
$SOLUTION_PATH = "$WORKSPACE_PATH\Web-$BuildType"

function InstallEmscripten() {
    
    Set-Location $WORKSPACE_PATH

    if (Test-Path -Path "$EMSDK_PATH") {
        if (-not (Test-Path -Path "$EMSDK_PATH/install.success")) {
            # if the last clone failed, clean up
            Write-Host "`nPreviews EMSDK installation failed, cleaning up." -ForegroundColor Yellow
            Remove-Item -Path "$EMSDK_PATH" -Recurse -Force
        }
    }

    if (-not (Test-Path -Path $EMSDK_PATH)) {
        
        Write-Host "`nDownloading EMSDK." -ForegroundColor Yellow

        git clone https://github.com/emscripten-core/emsdk.git
        
        if (!$?) {
            Remove-Item -Path "$EMSDK_PATH" -ErrorAction Ignore
            throw "Downloading EMSDK failed. Try again."
        }

        Write-Host "Finished downloading EMSDK.`n"

        Set-Variable -Name RerunCMake -Value $True
    }
    
    if (-not (Test-Path -Path "$EMSDK_PATH/install.success")) {
        
        Write-Host "`nInstalling Emscripten toolchain version $EMSCRIPTEN_VERSION." -ForegroundColor Yellow
        emsdk\emsdk.ps1 install $EMSCRIPTEN_VERSION
    
        New-Item "$EMSDK_PATH/install.success" -ErrorAction SilentlyContinue

        Write-Host "Finished installing EMSDK.`n"
    }
    
    if (!$?) {
        throw "Installing Emscripten failed. If this happens repeatedly, try deleting the EMSDK folder '$EMSDK_PATH'."
    }
}

function ActivateEmscripten() {

    Write-Host "`nActivating Emscripten toolchain version $EMSCRIPTEN_VERSION." -ForegroundColor Magenta

    Set-Location $WORKSPACE_PATH

    emsdk\emsdk.ps1 activate $EMSCRIPTEN_VERSION 

    if (!$?) {
        throw "Activating Emscripten failed."
    }
}

function GenerateCMakeSolution() {

    if ((-not ($RerunCMake)) -and (Test-Path -Path $SOLUTION_PATH)) {
        Write-Host "`nSkippping CMake step. Use -RerunCMake to force this." -ForegroundColor Yellow
        return 
    }

    Write-Host "`nGenerating Solution..." -ForegroundColor Magenta

    $CMAKE_ARGS = @("-S", "$SDK_PATH")
    
    $CMAKE_ARGS += "-G"
    $CMAKE_ARGS += "Ninja"
    $CMAKE_ARGS += "-B"
    $CMAKE_ARGS += $SOLUTION_PATH
    
    $CMAKE_ARGS += "-DCMAKE_TOOLCHAIN_FILE=$EMSDK_PATH\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake"

    $CMAKE_ARGS += "-DEZ_SOLUTION_NAME:STRING='Web'"
    $CMAKE_ARGS += "-DEZ_BUILD_EXPERIMENTAL_WEBGPU:BOOL=ON" # need WebGPU
    
    # disable lots of stuff that we don't need yet, for a minimal build
    $CMAKE_ARGS += "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF" # slower builds, but better error messages
    $CMAKE_ARGS += "-DEZ_ENABLE_QT_SUPPORT:BOOL=OFF" # disable QT
    $CMAKE_ARGS += "-DEZ_BUILD_FILTER='Everything'"
    $CMAKE_ARGS += "-DCMAKE_BUILD_TYPE=$BuildType" # Debug build for now
    $CMAKE_ARGS += "-DEZ_BUILD_FMOD:BOOL=OFF" # FMOD not yet supported
    # $CMAKE_ARGS += "-DEZ_BUILD_RMLUI:BOOL=OFF" # currently not necessary to set this variable
    $CMAKE_ARGS += "-DEZ_BUILD_SAMPLES:BOOL=ON"
    $CMAKE_ARGS += "-DEZ_BUILD_UNITTESTS:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_ADS_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_ASSIMP_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_DUKTAPE_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_ENET_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_IMGUI_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_JOLT_SUPPORT:BOOL=OFF" # Jolt not yet supported
    $CMAKE_ARGS += "-DEZ_3RDPARTY_KRAUT_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_LUA_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_OZZ_SUPPORT:BOOL=ON"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_RECAST_SUPPORT:BOOL=OFF"
    $CMAKE_ARGS += "-DEZ_3RDPARTY_TINYEXR_SUPPORT:BOOL=OFF"

    Write-Host ""
    Write-Host "Running cmake.exe $CMAKE_ARGS"
    Write-Host ""
    & $SDK_PATH/Data/Tools/Precompiled/cmake/bin/cmake.exe $CMAKE_ARGS
    
    if (!$?) {
        throw "CMake failed with exit code '$LASTEXITCODE'."
    }
    
    Write-Host "Finished generating Solution.`n" -ForegroundColor Magenta
}

function BuildSolution() {

    Write-Host "`nBuilding..." -ForegroundColor Magenta

    Set-Location $SOLUTION_PATH

    if ($Clean) {
        ninja clean
    }
    else {
        ninja
    }
    
    if (!$?) {
        throw "Build failed."
    }
}

$cwd = Get-Location

try {
    
    InstallEmscripten
    ActivateEmscripten
    GenerateCMakeSolution
    BuildSolution
    Write-Host "`nWeb build finished successfully.`n" -ForegroundColor Green
}
catch {
    Write-Host "`nWeb build failed.`n" -ForegroundColor Red
}
finally {
    Set-Location $cwd
}


