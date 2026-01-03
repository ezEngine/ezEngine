<#
.SYNOPSIS
  Export Android-related environment variables for the repository.

.DESCRIPTION
  Dot-source this script to export `ANDROID_HOME`, `ANDROID_NDK_HOME` and `JAVA_HOME`
  into the current PowerShell session. The script computes the repository root by
  walking two directories up from the script location.

.USAGE
  . .\SetupAndroidEnvVars.ps1
#>

param()

if ($MyInvocation.InvocationName -ne '.') {
    Write-Host "Please dot-source this script to export variables in the current session:" -ForegroundColor Yellow
    Write-Host "  . $PSScriptRoot\SetupAndroidEnvVars.ps1" -ForegroundColor Cyan
    return
}

# Compute repository root (two levels up from the script directory)
$repoRoot = Split-Path -Path (Split-Path -Path $PSScriptRoot -Parent) -Parent

# Defaults matching the pipeline layout
$androidHome = Join-Path $repoRoot "Workspace\shared\android"
$androidNdkHome = Join-Path $androidHome "ndk\26.1.10909125"
$javaHome = Join-Path $androidHome "jdk17"

$env:ANDROID_HOME = $androidHome
$env:ANDROID_NDK_HOME = $androidNdkHome
$env:JAVA_HOME = $javaHome

# Add platform-tools (adb) to PATH if not already present
$platformTools = Join-Path $androidHome 'platform-tools'
if ($env:Path -notlike "*$platformTools*") {
  $env:Path = "$platformTools;$env:Path"
}

Write-Host "Exported environment variables:" -ForegroundColor Green
Write-Host "  ANDROID_HOME=$env:ANDROID_HOME"
Write-Host "  ANDROID_NDK_HOME=$env:ANDROID_NDK_HOME"
Write-Host "  JAVA_HOME=$env:JAVA_HOME"
Write-Host "  PATH=$env:Path"
