param (
    [Parameter(Mandatory=$true)]
    [string]$SourceFolder
)

# Normalize the path
$SourceFolder = $SourceFolder.TrimEnd('\', '/')
$SourceFolder = Resolve-Path $SourceFolder -ErrorAction Stop

# Determine the test project folder name
$sourceFolderName = Split-Path -Leaf $SourceFolder
$parentFolder = Split-Path -Parent $SourceFolder
$testProjectName = "${sourceFolderName}HeaderCheck"
$testProjectFolder = Join-Path -Path $parentFolder -ChildPath $testProjectName

Write-Host "Source folder: $SourceFolder"
Write-Host "Test project folder: $testProjectFolder"
Write-Host ""

# Create the test project folder
if (Test-Path -Path $testProjectFolder) {
    Write-Host "Removing existing test project folder..."
    Remove-Item -Path $testProjectFolder -Recurse -Force
}

New-Item -ItemType Directory -Path $testProjectFolder | Out-Null
Write-Host "Created test project folder: $testProjectFolder"
Write-Host ""

# Find all .h files, excluding those with "implementation" in the path
Write-Host "Scanning for header files..."
$headerFiles = Get-ChildItem -Path $SourceFolder -Filter *.h -Recurse -File | Where-Object {
    $_.FullName -inotmatch "\\implementation\\" -and $_.FullName -inotmatch "/implementation/" -and $_.FullName -inotmatch "\\DirectXTex\\" -and $_.FullName -inotmatch "/DirectXTex/" -and $_.FullName -inotmatch "\\Platform\\" -and $_.FullName -inotmatch "/Platform/" -and $_.Name -inotlike "*_inl.h"
}

Write-Host "Found $($headerFiles.Count) header files (excluding implementation folders)"
Write-Host ""

# Generate a .cpp file for each header
$cppFiles = @()
$index = 1
foreach ($header in $headerFiles) {
    # Calculate the relative path from source folder to the header
    $relativePath = $header.FullName.Substring($SourceFolder.Length + 1)
    $relativePath = $relativePath -replace '\\', '/'

    # Prepend the source folder name to create the full include path
    $includePathWithFolder = "$sourceFolderName/$relativePath"

    # Create a unique cpp filename based on the header path
    # Replace path separators with underscores to create a flat structure
    $cppFileName = $relativePath -replace '/', '_' -replace '\.h$', '.cpp'
    $cppFilePath = Join-Path -Path $testProjectFolder -ChildPath $cppFileName

    # Generate the cpp file content
    $cppContent = "// Auto-generated test file for: $includePathWithFolder`n`n#include <$includePathWithFolder>`n"

    # Write the cpp file
    Set-Content -Path $cppFilePath -Value $cppContent -NoNewline
    $cppFiles += $cppFileName

    Write-Host "[$index/$($headerFiles.Count)] Generated: $cppFileName"
    $index++
}

Write-Host ""
Write-Host "Generated $($cppFiles.Count) .cpp test files"
Write-Host ""

# Generate CMakeLists.txt
Write-Host "Generating CMakeLists.txt..."

$cmakeContent = @"
ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME `${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY `${PROJECT_NAME} NO_UNITY NO_PCH)

target_link_libraries(`${PROJECT_NAME}
  PRIVATE
  $sourceFolderName
)
"@

$cmakeFilePath = Join-Path -Path $testProjectFolder -ChildPath "CMakeLists.txt"
Set-Content -Path $cmakeFilePath -Value $cmakeContent

Write-Host "Created CMakeLists.txt"
Write-Host ""
Write-Host "Header check project generation complete!"
Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Add 'add_subdirectory($testProjectName)' to $parentFolder\CMakeLists.txt"
Write-Host "2. Re-run CMake to include the new test project"
Write-Host "3. Build the '$testProjectName' target to verify all headers compile independently"
