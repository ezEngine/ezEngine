. "$PSScriptRoot\common-functions.ps1"

$appPath = Find-EditorProcessor
Write-Host "Using $appPath"

$failed = @()

# Transform all assets
Get-ChildItem -Path $PSScriptRoot\..\..\. -Filter ezProject -Recurse -File | ForEach-Object {
    $projectDir = $_.Directory.FullName

    # A project with C++ code needs its plugin built before its assets can be transformed: a scene that
    # uses a component from that plugin cannot be loaded without it, and the transform then produces
    # nothing for that scene. Without this the C++ samples ship with an empty asset cache.
    $arguments = @("-project", $projectDir, "-transform", "Default")

    if (Test-Path (Join-Path $projectDir "CppSource")) {
        $arguments += "-compile"
    }

    Write-Host "Transforming: $arguments"

    & $appPath @arguments | Out-Null

    if ($LASTEXITCODE -ne 0) {
        Write-Host "##[error]Failed with exit code $LASTEXITCODE : $projectDir"
        $failed += $projectDir
    }
}

if ($failed.Count -gt 0) {
    # Exiting with a failure here is deliberate: a project that does not transform produces a package
    # whose samples cannot be started, which is not something to find out after the release.
    Write-Host "##[error]$($failed.Count) project(s) failed to transform:"
    $failed | ForEach-Object { Write-Host "  $_" }
    exit 1
}

Write-Host "All projects transformed."
