param
(
    [string]$BinariesDir
)

# On Windows, if BinariesDir is not provided, default to the prebuilt binaries location
if ($IsWindows -and [string]::IsNullOrEmpty($BinariesDir)) {
    $BinariesDir = "$PSScriptRoot/../../Data/Tools/Precompiled"
}

if ([string]::IsNullOrEmpty($BinariesDir)) {
    throw "BinariesDir parameter is required. Set it to the directory containing ezShaderCompiler"
}

$RootDir = Split-Path -Path (Split-Path -Path $PSScriptRoot -Parent) -Parent

Write-Host "Compiling Android Shaders..." -ForegroundColor Green
$shaderCompilerPath = "$BinariesDir/ezShaderCompiler"
if ($IsWindows) {
    $shaderCompilerPath += ".exe"
}

if (-not (Test-Path $shaderCompilerPath)) {
    throw "ezShaderCompiler not found at: $shaderCompilerPath"
}

& $shaderCompilerPath -renderer Vulkan -platform VULKAN -project "$RootDir/Data/Base" -shader Shaders/Pipeline
if ($LASTEXITCODE -ne 0) { throw "ezShaderCompiler failed for Data/Base (exit code $LASTEXITCODE)." }

& $shaderCompilerPath -renderer Vulkan -platform VULKAN -project "$RootDir/Data/UnitTests" -shader "RendererTest/Shaders"
if ($LASTEXITCODE -ne 0) { throw "ezShaderCompiler failed for Data/UnitTests (exit code $LASTEXITCODE)." }

& $shaderCompilerPath -renderer Vulkan -platform VULKAN -project "$RootDir/Data/Samples/ShaderExplorer" -shader Shaders
if ($LASTEXITCODE -ne 0) { throw "ezShaderCompiler failed for Data/Samples/ShaderExplorer (exit code $LASTEXITCODE)." }
