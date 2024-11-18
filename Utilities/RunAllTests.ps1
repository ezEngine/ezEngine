#!/usr/bin/env powershell
param 
(
    [Parameter(Mandatory = $True, HelpMessage="Which build to use.")]
    [ValidateSet('Debug', 'Dev', 'Shipping')][string] $BuildType,
    [switch] $SkipFoundationTest,
    [switch] $SkipCoreTest,
    [switch] $SkipToolsFoundationTest,
    [switch] $SkipRendererTest,
    [switch] $SkipGameEngineTest,
    [switch] $SkipEditorTest
)

$Path = "$PSScriptRoot/../Output/Bin/WinVs2022$($BuildType)64"

function RunTest($name) {

    Write-Host "`nRunning $name.`n" -ForegroundColor Yellow

    & "$Path\$name.exe" -nosave -all -nogui

    if (!$?) {
        Write-Host "`n$name failed`n" -ForegroundColor Yellow
        throw
    }

    Write-Host "`n$name succeeded.`n" -ForegroundColor Green
}

if (-not $SkipFoundationTest) {
    RunTest "FoundationTest"
}

if (-not $SkipCoreTest) {
    RunTest "CoreTest"
}

if (-not $SkipToolsFoundationTest) {
    RunTest "ToolsFoundationTest"
}

if (-not $SkipRendererTest) {
    RunTest "RendererTest"
}

if (-not $SkipGameEngineTest) {
    RunTest "GameEngineTest"
}

if (-not $SkipEditorTest) {
    RunTest "EditorTest"
}
