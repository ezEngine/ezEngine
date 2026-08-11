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

# Remove the FMOD Studio authoring project. It is the source from which the sound banks are built,
# so it is only of interest to someone who wants to edit the sounds themselves. The runtime data is
# in Data\Content\Sound\Soundbanks, and the sound bank assets reference only that.
#
# The licensing files of the Sonniss sound library are kept, at their original paths, because the
# documentation points at them (see the Sonniss section of the third-party code page).
$fmodProjectDir = Join-Path $RootDir "Data\Content\Sound\FmodProject"

$fmodFilesToKeep = @(
    "Assets\Sonnis\Licensing.pdf",
    "Assets\Sonnis\README.txt"
)

if (Test-Path $fmodProjectDir)
{
    Write-Host "Removing the FMOD authoring project..."

    $keepFullPaths = $fmodFilesToKeep | ForEach-Object { (Join-Path $fmodProjectDir $_) }

    Get-ChildItem $fmodProjectDir -Recurse -File | Where-Object { $keepFullPaths -notcontains $_.FullName } | ForEach-Object {
        Remove-PathIfExists $_.FullName
    }

    # Delete the directories that just became empty. Repeat, so that parents of removed directories
    # are picked up as well.
    if (-not $WhatIfPreference)
    {
        for ($i = 0; $i -lt 8; $i++)
        {
            $empty = Get-ChildItem $fmodProjectDir -Recurse -Directory | Where-Object { $null -eq (Get-ChildItem $_.FullName -Force | Select-Object -First 1) }

            if (-not $empty)
            {
                break
            }

            $empty | ForEach-Object { Remove-Item -Recurse -Force $_.FullName }
        }
    }
}

# Strip the precompiled tools down to what a release package actually needs.
#
# What has to be kept, because it does not exist in Output\Bin:
#   cmake             - builds the C++ plugin of a project. ezCppProject::GetCMakePath() resolves to
#                       'cmake/bin/cmake', which is only found below the precompiled tools folder.
#   7z.exe / 7z.dll   - the editor runs 7z to unpack the archives inside a freshly cloned remote
#                       project (ExtractArchivesInDirectory in EditorApp.cpp). Also referenced by
#                       EZ_CONFIG_PATH_7ZA in ezCMakeConfig.cmake.
#
# tracy-profiler.exe is not kept. The editor can launch it to profile a running game, but that is a
# development tool, and it is a large binary. Without it the editor's Tracy action fails to find the
# executable; anyone who wants it can get it from the repository or from upstream.
$precompiledDir = Join-Path $RootDir "Data\Tools\Precompiled"

$precompiledToKeep = @(
    "cmake",
    "7z.exe",
    "7z.dll",
    "LICENSE.md"
)

if (Test-Path $precompiledDir)
{
    Get-ChildItem $precompiledDir | Where-Object { $precompiledToKeep -notcontains $_.Name } | ForEach-Object {
        Remove-PathIfExists $_.FullName
    }
}

# Remove cmake-gui.exe and ctest.exe. The executables are in the 'bin' subdirectory.
$cmakeBinDir = Join-Path $precompiledDir "cmake\bin"
Remove-PathIfExists (Join-Path $cmakeBinDir "cmake-gui.exe")
Remove-PathIfExists (Join-Path $cmakeBinDir "ctest.exe")

# Remove tools from the binary output that are only useful when working on the engine itself.
# ezStackResolver needs the PDB files of the build to resolve a callstack, and those are not part
# of a release package.
# Note: ezMiniDumpTool must NOT be removed. The crash handler launches it out-of-process to write
# the crash dump (see ezMiniDumpUtils::LaunchMiniDumpTool), and it looks for the executable next to
# the crashed application. The resulting dump can still be resolved by whoever has the PDBs.
$binDir = Join-Path $RootDir "Output\Bin"

$binariesToRemove = @(
    "DependencyAnalysis.exe",
    "GdbProxy.exe",
    "HeaderCheck.exe",
    "LineCount.exe",
    "ezStaticLinkUtil.exe",
    "ezStackResolver.exe"
)

if (Test-Path $binDir)
{
    Get-ChildItem $binDir -Directory | ForEach-Object {
        foreach ($binary in $binariesToRemove)
        {
            Remove-PathIfExists (Join-Path $_.FullName $binary)
        }
    }
}

# The Qt plugin subdirectories next to the binaries are copied as a whole, so they contain both the
# release and the debug build of every plugin. Qt names the debug build 'xyzd.dll'. A Dev or
# Shipping build links against the release Qt and never loads those, so drop them.
# In a Debug output folder it is the other way around, therefore that one is left alone.
$qtPluginDirs = @(
    "iconengines",
    "imageformats",
    "platforms"
)

if (Test-Path $binDir)
{
    Get-ChildItem $binDir -Directory | Where-Object { $_.Name -notmatch "Debug" } | ForEach-Object {
        foreach ($pluginDir in $qtPluginDirs)
        {
            $dir = Join-Path $_.FullName $pluginDir

            if (-not (Test-Path $dir))
            {
                continue
            }

            # Only remove a debug DLL when the matching release DLL is there, so that a plugin that
            # happens to end in 'd' is never mistaken for a debug build.
            Get-ChildItem $dir -Filter *d.dll -File | Where-Object {
                Test-Path (Join-Path $dir ($_.BaseName.Substring(0, $_.BaseName.Length - 1) + ".dll"))
            } | ForEach-Object {
                Remove-PathIfExists $_.FullName
            }
        }
    }
}

# Remove the .exp files next to the import libraries. The linker writes one for every DLL that
# exports symbols, but only the .lib is needed to link against those DLLs. Nothing in
# Output\Bin\ezExport.cmake refers to them.
$libDir = Join-Path $RootDir "Output\Lib"

if (Test-Path $libDir)
{
    Get-ChildItem $libDir -Filter *.exp -Recurse -File | ForEach-Object {
        Remove-PathIfExists $_.FullName
    }
}

# '.loaded' files are written at runtime next to a plugin DLL, to track which copy of it is in use.
Get-ChildItem $RootDir -Filter *.loaded -Recurse -File | ForEach-Object {
    Remove-PathIfExists $_.FullName
}

# The package is built for x64 Windows, so the freetype builds for the other architectures are dead
# weight. RmlUi picks the directory by pointer size (Code\ThirdParty\RmlUi\Source\Core\CMakeLists.txt).
$freetypeDir = Join-Path $RootDir "Code\ThirdParty\RmlUi\Dependencies\freetype\win"

Remove-PathIfExists (Join-Path $freetypeDir "x86")
Remove-PathIfExists (Join-Path $freetypeDir "arm64")

# Remove embree. It is only used by the BakingPlugin, which is not part of the package.
# The whole directory has to go, not just the binaries: FindEzEmbree.cmake detects embree by looking
# for 'include/embree3/rtcore.h', and ez_requires_embree() skips the plugin when that fails. Leaving
# the headers behind would make the plugin configure and then fail to link.
Remove-PathIfExists (Join-Path $RootDir "Code\ThirdParty\embree")
