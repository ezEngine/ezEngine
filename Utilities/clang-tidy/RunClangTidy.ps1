param
(
    [string]
    $Workspace = "Workspace/clang-tidy",
    [string]
    $Checks = "-*,ez-name-check,modernize-use-default-member-init,modernize-use-equals-default,modernize-use-using",
    [string]
    $ChecksGroup1 = "clang-analyzer-core.*",
    [string]
    $ExcludeRootFiles = "DirectXTex|ThirdParty|\.rc$|qrc_resources.cpp$",
    [string]
    $Group1Pattern = "Code\\Engine\\|Code\\EnginePlugins\\|Code\\Editor\\|Code\\EditorPlugins\\",
    [string]
    $HeaderPattern = "^((?!ThirdParty|DirectXTex|ogt_vox|ui_).)*$",
    [string]
    $LlvmInstallDir,
    [string]
    $TempDir,
    [string]
    $ClangTidy = "$PSScriptRoot\..\..\Data\Tools\Precompiled\clang-tidy\clang-tidy.exe",
    [string]
    $DependencyAnalysis = "$PSScriptRoot\..\..\Data\Tools\Precompiled\DependencyAnalysis.exe",
    [string]
    $LogFile,
    [string]
    $DiffTo = "origin/dev",
    [string]
    $SingleFile,
    [int]
    $FileLimit = 0,
    [string]
    $FilterPattern,
    [switch]
    $Vso
)

$ErrorActionPreference = "Stop"

Set-Location (Join-Path -Path $PSScriptRoot -ChildPath "..\..")

if (-not $LlvmInstallDir) {
    Write-Host "LlvmInstallDir is not set. Searching for clang-apply-replacements.exe in default paths..."
    $possiblePaths = @((Join-Path $pwd "llvm"), "C:\Program Files\LLVM")
    foreach ($path in $possiblePaths) {
        $testPath = Join-Path $path "bin\clang-apply-replacements.exe"
        if (Test-Path $testPath) {
            $LlvmInstallDir = $path
            Write-Host "LlvmInstallDir set to $LlvmInstallDir"
            break
        }
    }
    if (-not $LlvmInstallDir) {
        Write-Error "LlvmInstallDir is not set. Please run SetupWorkspace.ps1."
        exit 1
    }
}

$Workspace = (Resolve-Path $Workspace).Path

function New-TemporaryDirectory {
    $tempFolderPath = Join-Path $Env:Temp $(New-Guid)
    return (New-Item -Type Directory -Path $tempFolderPath).FullName
}

if(!(Test-Path $Workspace/compile_commands.json))
{
    Write-Error "Could not find compile commands database at $Workspace/compile_commands.json. Please run SetupWorkspace.ps1."
    exit 1
}

$ClangApplyReplacements = Join-Path $LlvmInstallDir "bin\clang-apply-replacements.exe"

if(!(Test-Path $ClangApplyReplacements))
{
    Write-Error "Could not find clang-apply-replacements binary in $ClangApplyReplacements. Check LlvmInstallDir parameter."
    exit 1
}

$ClangLibPath = Join-Path $LlvmInstallDir "lib\clang"

if(!(Test-Path $ClangLibPath))
{
    Write-Error "Could not find $ClangLibPath. Check LlvmInstallDir parameter."
    exit 1
}

$ClangLibPathSub = @((Get-ChildItem -Directory $ClangLibPath).Name)
if($ClangLibPathSub.Length -eq 0)
{
    Write-Error "There are not subdirectories in $ClangLibPath. Check LlvmInstallDir parameter."
    exit 1
}

$ClangLibPath = Join-Path $ClangLibPath $ClangLibPathSub[0] "include"
Write-Host "Clang include path $ClangLibPath"

$DeleteTempDir = $false
if(!$TempDir)
{
    $TempDir = New-TemporaryDirectory
    $DeleteTempDir = $true
    Write-Host "Temporary Directory is $TempDir"
}

$files = @()

if($SingleFile)
{
    $files = @($SingleFile)
}
else 
{
    $files = @((Get-Content $Workspace\compile_commands.json | ConvertFrom-Json -Depth 3).file | ? {!($_ -match $ExcludeRootFiles)})
}

if($DiffTo -and [string]::IsNullOrEmpty($SingleFile))
{
    # Get list of changed files from git
    $mergeBase = git merge-base HEAD "$DiffTo"
    if($lastexitcode -ne 0)
    {
        Write-Error "Git merge-base failed: $mergeBase"
        exit 1
    }
    
    $diffFiles = @(git diff --name-only HEAD $mergeBase) -replace "/","\"
    if($lastexitcode -ne 0)
    {
        Write-Error "Git diff failed: $diffFiles"
        exit 1
    }
    
    Write-Host "Number of files changed to merge-base:" $diffFiles.Length
    
    # Build a hashmap of all git diff files
    $diffMap = @{}
    foreach($diffFile in $diffFiles)
    {
        $diffMap[$diffFile] = $true
    }
    
    # Filter files to clang-tidy by the files given by git diff
    $files = @($files | ? { 
        if($_.StartsWith($Workspace))
        {
            $_ = $_.Substring($Workspace.Length)
        }
        elseif($_.StartsWith($pwd.Path))
        {
            $_ = $_.Substring($pwd.Path.Length)
        }
        $_ = $_.trim("\")
        
        if($_ -match "moc_.*\.cpp$")
        {
            $mocCpp = $_
            $mocH1 = $_ -replace "cpp$","h"
            $path = Split-Path -Parent -Path $mocH1
            $name = Split-Path -Leaf -Path $mocH1
            $name = $name.trim("moc_")
            $moch2 = Join-Path $path $name
            return $diffMap.ContainsKey($mocCpp) -or $diffMap.ContainsKey($mocH1) -or $diffMap.ContainsKey($mocH2)
        } else {
            return $diffMap.ContainsKey($_)
        }
    })    

    $headersInDiff = $diffFiles | ? { $_.EndsWith(".h") -and (Test-Path $_) }
    if ($headersInDiff.Length -gt 0)
    {
        $originalNumFiles = $files.Length
        Write-Host "Looking for dependencies of all headers in diff"
        . $DependencyAnalysis -i "Qt6-6" -i "VulkanSDK" -i "ThirdParty" -i ".moc.cpp" -i "qrc_resources.cpp" -i "moc_" -o $Workspace/cpp_dependencies.json $Workspace/compile_commands.json
        if($lastexitcode -ne 0)
        {
            Write-Error "Dependency analysis failed. Command used: $DependencyAnalysis -i `"Qt6-6`" -i `"VulkanSDK`" -i `"ThirdParty`" -i `".moc.cpp`" -o $Workspace/cpp_dependencies.json $Workspace/compile_commands.json"
            exit 1
        }
        
        if(!(Test-Path $Workspace/cpp_dependencies.json))
        {
            Write-Error "Dependency analysis did not write expected output file $Workspace/cpp_dependencies.json"
            exit 1
        }
        
        $cppDependencies = (Get-Content $Workspace/cpp_dependencies.json | ConvertFrom-Json -Depth 4).files
        $headerDependencies = @{}
        foreach ($cpp in $cppDependencies)
        {
            foreach ($dep in $cpp.dependencies)
            {
                $headerDependencies[$dep] += @($cpp.name)
            }
        }
        $headerDependencies | ConvertTo-Json | Out-File test.json
        
        foreach($header in $headersInDiff)
        {
            $absHeader = (Resolve-Path $header) -replace "\\","/"
            if($headerDependencies.ContainsKey($absHeader))
            {
                foreach($cppFile in $headerDependencies[$absHeader])
                {
                   $path = $cppFile -replace "/","\"
                   $files += @($path)
                }
            }
            else
            {
                Write-Host "Warning: No dependency information for $absHeader" -foreground yellow
            }
        }
        $files = $files | Sort-Object | Get-Unique | ? {!($_ -match $ExcludeRootFiles)}
        $numFilesAdded = $files.Length - $originalNumFiles
        if($numFilesAdded -gt 0)
        {
            Write-Host "Added $numFilesAdded cpp files due to header changes"
        }
    }
}

& $ClangTidy "--checks=$Checks,$ChecksGroup1" --list-checks
if($lastexitcode -ne 0)
{
    Write-Error "Inital clang-tidy test run failed"
    exit 1
}

$ninjaFilePath = Join-Path $Workspace "build.ninja"
if(!(Test-Path $ninjaFilePath))
{
    Write-Error "Could not find ninja makefile at $ninjaFilePath. Check the Workspace parameter."
    exit 1
}
$ninjaFile = Get-Content $ninjaFilePath

$uiCmds = @()

$uiHFiles = (sls "^build ([^ ]*.h) \|" $ninjaFilePath | % { @{"file" = $_.Matches[0].Groups[1].Value; "line" = $_.LineNumber }})
foreach($uiHFile in $uiHFiles)
{
    $cmd = $ninjaFile[$uiHFile.line]
    if(!($cmd -match "\s*COMMAND = (.*)$"))
    {
        Write-Error "Unexpected layout of ninja makefile for $($uiHFile.cpp):' $cmd'"
        exit 1
    }
    $uiCmds += $Matches[1]
}

if($uiCmds.Length -gt 0)
{
    Write-Host "Running ui on $($uiCmds.Length) files"

    $uiJob = $uiCmds | Foreach-Object -Parallel `
    {
       (Invoke-Expression $_ 2>&1) | Out-String
    } -AsJob -ThrottleLimit $env:NUMBER_OF_PROCESSORS

    try
    { 
       $totalItems = @($uiJob.ChildJobs).Length
       while($uiJob.State -ne "Completed" -and $uiJob.State -ne "Failed")
       {
           $jobsLeft = ($uiJob.ChildJobs | ? {$_.State -eq "NotStarted"}).Length
           $jobsFinished = $totalItems - $jobsLeft
           $percent = [int]($jobsFinished / $totalItems * 100)
           Write-Progress -Activity "Running ui on source files" -Status "$jobsFinished of $totalItems" -PercentComplete $percent -Verbose
           Start-Sleep -Milliseconds 250
       }
       Write-Progress -Activity "Running ui on source files" -Completed  -Verbose
       $uiJob | Receive-Job -Wait | ? {$_.Length -gt 0 -or -not ($_ -match "^\s*$")} | % { "'" + $_ + "'" }
    }
    finally
    {
       $uiJob | Remove-Job -Force
    }
}


$mocCmds = @{}

$mocCppFiles = (sls "build ([^ ]*/moc_[^ ]*\.cpp) \|" $ninjaFilePath | % {@{ "cpp" = $_.Matches[0].Groups[1].Value; "line" = $_.LineNumber }})
foreach($mocCppFile in $mocCppFiles)
{
    $cmd = $ninjaFile[$mocCppFile.line]
    if(!($cmd -match "\s*COMMAND = (.*)$"))
    {
        Write-Error "Unexpected layout of ninja makefile for $($mocCppFile.cpp):' $cmd'"
        exit 1
    }
    $cmd = $Matches[1]
    $path = Join-Path $Workspace ($mocCppFile.cpp -replace "/","\")
    $mocCmds[$path] = $cmd
}

$mocCmdsToExecute = @()
foreach($file in $files)
{
    if($mocCmds.ContainsKey($file))
    {
        $mocCmdsToExecute += $mocCmds[$file]
    }
}

if($mocCmdsToExecute.Length -gt 0)
{
    Write-Host "Running moc on $($mocCmdsToExecute.Length) files"

    $mocJob = $mocCmdsToExecute | Foreach-Object -Parallel `
    {
       (Invoke-Expression $_ 2>&1) | Out-String
    } -AsJob -ThrottleLimit $env:NUMBER_OF_PROCESSORS

    try
    { 
       $totalItems = @($mocJob.ChildJobs).Length
       while($mocJob.State -ne "Completed" -and $mocJob.State -ne "Failed")
       {
           $jobsLeft = ($mocJob.ChildJobs | ? {$_.State -eq "NotStarted"}).Length
           $jobsFinished = $totalItems - $jobsLeft
           $percent = [int]($jobsFinished / $totalItems * 100)
           Write-Progress -Activity "Running moc on source files" -Status "$jobsFinished of $totalItems" -PercentComplete $percent -Verbose
           Start-Sleep -Milliseconds 250
       }
       Write-Progress -Activity "Running moc on source files" -Completed  -Verbose
       $mocJob | Receive-Job -Wait | ? {$_.Length -gt 0 -or -not ($_ -match "^\s*$")} | % { "'" + $_ + "'" }
    }
    finally
    {
       $mocJob | Remove-Job -Force
    }
}

if($files.Length -eq 0)
{
    Write-Host "No files to run clang-tidy on. All good."
    exit 0
}

$syncStore = [hashtable]::Synchronized(@{})
$syncStore.NumErrors = 0
$syncStore.NumMessages = 0

if($FilterPattern)
{
    $files = $files | ? {$_ -match $FilterPattern}
    Write-Host $files
}

if($FileLimit -gt 0)
{
    $files = $files[0..($FileLimit - 1)]
}

Write-Host "Running clang-tidy on" $files.Length "files"

$warningSeen = [System.Collections.Concurrent.ConcurrentDictionary[string, object]]::new()

$job = $files | Foreach-Object -Parallel `
{
   $ClangTidy = $using:ClangTidy
   $TempDir = $using:TempDir
   $Workspace = $using:Workspace
   $Checks = $using:Checks
   $HeaderPattern = $using:HeaderPattern
   $syncStore = $using:syncStore
   $ClangLibPath = $using:ClangLibPath
   $warningSeen = $using:warningSeen
   $ChecksGroup1 = $using:ChecksGroup1
   $Group1Pattern = $using:Group1Pattern
   
   if($Group1Pattern -and $_ -match $Group1Pattern)
   {
       $Checks += ",$ChecksGroup1"
   }
   
   $fixesFile = Join-Path $TempDir "$(New-Guid).yaml"
   
   $output += "////////////////////////////////////////////////////////////////////////////////////////////////////////////`r`n"
   $output += "// $_`r`n"
   $output += "// $ClangTidy -p $Workspace --checks=$Checks `"--header-filter=$HeaderPattern`" --extra-arg=-DBUILDSYSTEM_CLANG_TIDY=1 `"--extra-arg=-isystem$ClangLibPath`" $_ `r`n"
   $output += "////////////////////////////////////////////////////////////////////////////////////////////////////////////`r`n"
   $tidyOutput = (& $ClangTidy -p $Workspace --checks=$Checks --header-filter=$HeaderPattern --export-fixes=$fixesFile --extra-arg=-DBUILDSYSTEM_CLANG_TIDY=1 "--extra-arg=-isystem$ClangLibPath" $_ *>&1) | Out-String -Stream
   
   $filteredTidyOutput = @($tidyOutput | ? {!$_.StartsWith("Suppressed") -and !$_.StartsWith("Use -header-filter") -and !($_ -match "warnings generated.") -and $_.trim() -ne "" })
   $filteredTidyOutputWithIndex = @($filteredTidyOutput | % {$i=0} {$value = @{msg = $_; index = $i}; $i++; return $value})
   $finalTidyOutput = @($filteredTidyOutputWithIndex | % {if($_.msg -match "^(.*(h|cpp|hpp)):[0-9]+:[0-9]+: (warning|error):") { @{msg = $_.msg; index = $_.index; filename = $matches[1]}}})
   
   $numMessages = 0
   # Go through all warnings in the clang-tidy output and filter them for relevant warnings.
   if($finalTidyOutput.length -gt 0)
   {
       0..($finalTidyOutput.length-1) | % {
         $curWarning = $finalTidyOutput[$_]
         # We don't care about warnins in moc files
         if($curWarning.filename.EndsWith(".moc.cpp") -or $curWarning.filename -match "moc_.*cpp")
         {
             return
         }
         # Have we printed this warning before?
         if(!$warningSeen.TryAdd($curWarning.msg, $true))
         {
             return
         }
         $syncStore.NumMessages++
         $numMessages++
         $endIndex = if($_ + 1 -lt $finalTidyOutput.length) { $finalTidyOutput[$_ + 1].index } else { $filteredTidyOutput.length}
         
         ($curWarning.index)..($endIndex-1) | % { $output += $filteredTidyOutput[$_] + "`r`n" }
       }
   }
   
   if($lastexitcode -ne 0)
   {
       $syncStore.NumErrors++
       if($numMessages -eq 0) # If we didn't find any errors or warnings but compilation still failed, just forward the entire output.
       {
         $numMessages++
         $output += $tidyOutput
       }
   }
   
   if($numMessages -gt 0)
   {
     return $output
   }
   else 
   {
     return
   }
} -AsJob -ThrottleLimit $env:NUMBER_OF_PROCESSORS

try
{ 
   $totalItems = @($job.ChildJobs).Length
   while($job.State -ne "Completed" -and $job.State -ne "Failed")
   {
       $jobsLeft = ($job.ChildJobs | ? {$_.State -eq "NotStarted"}).Length
       $jobsFinished = $totalItems - $jobsLeft
       $percent = [int]($jobsFinished / $totalItems * 100)
       if($Vso)
       {
           Write-Host "##vso[task.setprogress value=$percent;]Running clang-tidy on source files: $jobsFinished of $totalItems"
       }
       else
       {
           Write-Progress -Activity "Running clang-tidy on source files" -Status "$jobsFinished of $totalItems" -PercentComplete $percent -Verbose
       }
       Start-Sleep -Milliseconds 250
   }
   if($Vso)
   {
       Write-Host "##vso[task.setprogress value=100;]Running clang-tidy on source files done."
   }
   else
   {
       Write-Progress -Activity "Running clang-tidy on source files" -Completed  -Verbose
   }
   
   if($LogFile)
   {
       $job | Receive-Job -Wait 2>&1 | Out-File $LogFile
   }
   else 
   {
       $job | Receive-Job -Wait
   }
   
   if($syncStore.NumErrors -gt 0)
   {
       throw "$($syncStore.NumErrors) compilation units failed to compile. Please fix the compile errors."
   }

   if((Get-ChildItem $TempDir).Length -gt 0)
   {
       Write-Host "Applying clang-tidy suggested fixes..."
       & $ClangApplyReplacements --ignore-insert-conflict $TempDir
       if($lastexitcode -ne 0)
       {
           throw "clang-apply-replacements failed with error code $lastexitcode"
       }
   }
  
   if($syncStore.NumMessages -gt 0)
   {
       Write-Host "##vso[task.logissue type=error]clang-tidy issued $($syncStore.NumMessages) warnings / errors. Please fix these."
       Write-Host "##vso[task.complete result=Failed;]"
   }
}
finally
{
   $job | Remove-Job -Force
   if($DeleteTempDir)
   {
       Remove-Item -Recurse -Force $TempDir
   }
}