[CmdletBinding(SupportsShouldProcess = $True)]

param
(
    [string] $RootDir = "."
)

$RootDir = Resolve-Path $RootDir

function Remove-PathIfExists
{
    param ([string] $Path)

    if (Test-Path $Path)
    {
        if ($WhatIfPreference)
        {
            Write-Host "What if: Removing '$Path'"
        }
        else
        {
            Write-Host "Removing '$Path'"
            Remove-Item -Recurse -Force $Path
        }
    }
}

# Remove PDB files
Write-Host "Removing PDB files..."
Get-ChildItem $RootDir -Include *.pdb -Recurse | ForEach-Object {
    if ($WhatIfPreference)
    {
        Write-Host "What if: Removing '$($_.FullName)'"
    }
    else
    {
        Write-Host "Removing '$($_.FullName)'"
        Remove-Item $_
    }
}

# Remove Build output folders from sample projects
Write-Host "Removing sample build folders..."
Get-ChildItem "$RootDir\Data\Samples" -Filter Build -Recurse -Directory | ForEach-Object {
    if ($WhatIfPreference)
    {
        Write-Host "What if: Removing '$($_.FullName)'"
    }
    else
    {
        Write-Host "Removing '$($_.FullName)'"
        Remove-Item -Recurse -Force $_
    }
}

# Remove precompiled tool folders not needed in a release
$precompiledDir = Join-Path $RootDir "Data\Tools\Precompiled"

$foldersToRemove = @(
    "clang-format",
    "clang-tidy",
    "optipng",
    "webserver"
)

foreach ($folder in $foldersToRemove)
{
    Remove-PathIfExists (Join-Path $precompiledDir $folder)
}

# Remove individual precompiled files not needed in a release
$filesToRemove = @(
    "DependencyAnalysis.exe",
    "7z.exe",
    "7z.dll"
)

foreach ($file in $filesToRemove)
{
    Remove-PathIfExists (Join-Path $precompiledDir $file)
}

# Remove cmake-gui.exe and ctest.exe from the cmake directory
$cmakeDir = Join-Path $precompiledDir "cmake"
Remove-PathIfExists (Join-Path $cmakeDir "cmake-gui.exe")
Remove-PathIfExists (Join-Path $cmakeDir "ctest.exe")
