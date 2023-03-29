param(
    [string]$TargetBranch,
	[string]$ArtifactDirectory,
	[string]$Filter
)

# Execute the given command and catch any error
function exec
{
    [CmdletBinding()]
    param(
        [Parameter(Position=0,Mandatory=1)][scriptblock]$cmd,
        [Parameter(Position=1,Mandatory=0)][string]$errorMessage = ("Error executing command {0}" -f $cmd)
    )
    & $cmd
    if ($lastexitcode -ne 0)
    {
        throw ("Exec: " + $errorMessage)
    }
}

#always stop on any errors
$ErrorActionPreference = "Stop"

if(-not $Filter)
{
	$Filter = "*"
}

$refsHeadString = "refs/heads/"
if($TargetBranch.StartsWith($refsHeadString))
{
	$TargetBranch = $TargetBranch.SubString($refsHeadString.Length)
} 

# Find the common root commit to diff against
$baseCommit = exec({git merge-base HEAD origin/$TargetBranch})

# Run clang-format on the diff only
$diff = exec {cmd /C "git diff $baseCommit..HEAD -U0 --no-color -- $Filter" | python "$PSScriptRoot\..\Data\Tools\Precompiled\clang-format\clang-format-diff.py" -style file -p1 -binary "$PSScriptRoot\..\Data\Tools\Precompiled\clang-format\clang-format.exe"}

if($diff.Length -gt 0)
{
	echo $diff
	# Display error message
	$diffFile = "clangFormat.patch"
	Write-Host "##vso[task.logissue type=error]Clang format suggested changes. Clang format check failed! See artifact $diffFile for suggested changes. Apply patch with: git apply -p0 clangFormat.patch" -foreground red
	#Convert to linux line ending as required for a correct patch file
	$diff = $diff -join "`n"
	
	#Write out patch file
	$Utf8NoBomEncoding = New-Object System.Text.UTF8Encoding $False
	$absPath = "$(Resolve-Path $ArtifactDirectory)\$diffFile"
	[System.IO.File]::WriteAllText($absPath, "$diff`n", $Utf8NoBomEncoding)
	exit 1
}
else
{
	Write-Host "##vso[task.complete result=Succeeded;]Clang format check passed." -foreground green
}