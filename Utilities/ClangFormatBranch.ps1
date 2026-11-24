<#
.SYNOPSIS
Formats all C++ code that was changed on the current branch, the same way the ClangFormat CI step does.

.DESCRIPTION
Determines the merge-base between HEAD and the base branch and runs clang-format (through clang-format-diff.py)
on all lines that differ between that commit and the working tree. This includes committed, staged and unstaged
changes. Untracked (new, not ignored) source files are formatted entirely.

Only lines that were touched are formatted, so unrelated code in the same files stays as it is.

clang-format binary selection:
  - Windows: Data/Tools/Precompiled/clang-format/clang-format.exe
  - Linux:   Utilities/Ubuntu/clang-format (the binary used by CI)
  - Fallback: clang-format-18 or clang-format from PATH (a warning is printed if it is not version 18)

Works with PowerShell 7 (pwsh) on Windows and Linux. Requires git and Python 3.

.PARAMETER BaseBranch
The branch the current branch will be merged into. Default is 'dev'.
The script uses 'origin/<BaseBranch>' if it exists, otherwise the local branch. It does not fetch.

.PARAMETER BaseRef
Any git ref or commit to diff against (for example 'upstream/dev'). Overrides BaseBranch.

.PARAMETER Check
Don't modify any files, only print the changes clang-format would make. Exits with code 1 if anything needs formatting.

.PARAMETER NoUntracked
Ignore untracked files.

.EXAMPLE
pwsh Utilities/ClangFormatBranch.ps1

.EXAMPLE
pwsh Utilities/ClangFormatBranch.ps1 -Check
#>
param(
  [string]$BaseBranch = "dev",
  [string]$BaseRef,
  [switch]$Check,
  [switch]$NoUntracked
)

$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourceExtensions = @("*.cpp", "*.h", "*.inl")
$sourceRegex = '.*\.(cpp|h|inl)'

function Test-Command([string]$Command, [string[]]$Arguments)
{
  try
  {
    $null = & $Command @Arguments 2>&1
    return ($LASTEXITCODE -eq 0)
  }
  catch
  {
    return $false
  }
}

function Find-Python
{
  # on Windows 'python3' is often only the Microsoft Store stub, which fails the version check
  foreach ($candidate in @("python3", "python", "py"))
  {
    if ((Get-Command $candidate -ErrorAction SilentlyContinue) -and (Test-Command $candidate @("--version")))
    {
      return $candidate
    }
  }

  throw "Python 3 was not found. Install it and make sure 'python3' or 'python' is in PATH."
}

function Find-ClangFormat
{
  $candidates = @()

  if ($IsWindows -or ($PSVersionTable.PSEdition -eq "Desktop"))
  {
    $candidates += Join-Path $repoRoot "Data/Tools/Precompiled/clang-format/clang-format.exe"
  }
  elseif ($IsLinux)
  {
    $linuxBinary = Join-Path $repoRoot "Utilities/Ubuntu/clang-format"

    if (Test-Path $linuxBinary)
    {
      # the file is committed as executable, but a checkout with core.fileMode=false may lose the bit
      # only touch it when necessary, since changing the mode shows up as a modification in git
      & test -x $linuxBinary
      if ($LASTEXITCODE -ne 0)
      {
        & chmod +x $linuxBinary 2>&1 | Out-Null
      }
      $candidates += $linuxBinary
    }
  }

  foreach ($name in @("clang-format-18", "clang-format"))
  {
    $cmd = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cmd)
    {
      $candidates += $cmd.Source
    }
  }

  foreach ($candidate in $candidates)
  {
    if (-not (Test-Path $candidate))
    {
      continue
    }

    try
    {
      $version = & $candidate --version 2>&1
      if ($LASTEXITCODE -ne 0)
      {
        continue
      }
    }
    catch
    {
      continue
    }

    $version = "$version".Trim()
    Write-Host "Using $candidate ($version)"

    if ($version -notmatch "version 18\.")
    {
      Write-Warning "The repository uses clang-format 18. Other versions format some constructs differently than CI."
    }

    return $candidate
  }

  throw "No working clang-format binary found. Install clang-format 18 (e.g. 'sudo apt install clang-format-18')."
}

function Invoke-Git
{
  $result = & git -C $repoRoot @args
  if ($LASTEXITCODE -ne 0)
  {
    throw "git $args failed."
  }
  return $result
}

# git output and the input piped to python have to be UTF-8, otherwise non-ASCII paths get mangled
$prevOutputEncoding = $OutputEncoding
$prevConsoleEncoding = [Console]::OutputEncoding
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)

Push-Location $repoRoot

try
{
  # find the commit to diff against
  if (-not $BaseRef)
  {
    $BaseRef = $BaseBranch
    & git -C $repoRoot rev-parse --verify --quiet "refs/remotes/origin/$BaseBranch" | Out-Null
    if ($LASTEXITCODE -eq 0)
    {
      $BaseRef = "origin/$BaseBranch"
    }
  }

  $baseCommit = & git -C $repoRoot merge-base HEAD $BaseRef
  if ($LASTEXITCODE -ne 0 -or -not $baseCommit)
  {
    throw "Could not determine the merge-base between HEAD and '$BaseRef'. Pass -BaseRef or -BaseBranch."
  }

  Write-Host "Formatting changes since $BaseRef (merge-base $($baseCommit.Substring(0, 10)))"

  $python = Find-Python
  $clangFormat = Find-ClangFormat
  $diffScript = Join-Path $repoRoot "Data/Tools/Precompiled/clang-format/clang-format-diff.py"

  $formatArgs = @("-style", "file", "-p1", "-binary", $clangFormat, "-iregex", $sourceRegex)
  if (-not $Check)
  {
    $formatArgs += "-i"
  }

  # diff against the working tree, to include uncommitted changes as well
  $codeDiff = Invoke-Git diff $baseCommit -U0 --no-color --no-ext-diff -- @sourceExtensions

  $needsFormatting = $false

  if ($codeDiff)
  {
    $result = $codeDiff | & $python $diffScript @formatArgs

    # without -i, clang-format-diff.py exits with 1 when it prints a diff
    if ($LASTEXITCODE -ne 0 -and -not ($Check -and $result))
    {
      throw "clang-format-diff.py failed."
    }

    if ($Check -and $result)
    {
      $result | Write-Output
      $needsFormatting = $true
    }
  }

  if (-not $NoUntracked)
  {
    $untracked = @(Invoke-Git ls-files --others --exclude-standard -- @sourceExtensions)

    foreach ($file in $untracked)
    {
      if ($Check)
      {
        & $clangFormat -style=file --dry-run --Werror $file
        if ($LASTEXITCODE -ne 0)
        {
          $needsFormatting = $true
        }
      }
      else
      {
        & $clangFormat -style=file -i $file
        if ($LASTEXITCODE -ne 0)
        {
          throw "clang-format failed on '$file'."
        }
      }
    }
  }

  if ($Check)
  {
    if ($needsFormatting)
    {
      Write-Host "Code needs formatting. Run this script without -Check to apply it." -ForegroundColor Red
      exit 1
    }

    Write-Host "No formatting changes necessary." -ForegroundColor Green
  }
  else
  {
    Write-Host "Done." -ForegroundColor Green
  }
}
finally
{
  Pop-Location
  $OutputEncoding = $prevOutputEncoding
  [Console]::OutputEncoding = $prevConsoleEncoding
}
