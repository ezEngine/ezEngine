param(
    [switch]$installEmulator = $false,
    [switch]$acceptLicence = $false
)

# Require PowerShell 7+
if ($PSVersionTable.PSVersion.Major -lt 7) {
    Write-Host "This script requires PowerShell 7 or newer. Please run it using 'pwsh'." -ForegroundColor Red
    exit 1
}

# Import Android utils
. "$PSScriptRoot/AndroidUtils.ps1"

$sharedFolder = [System.IO.Path]::GetFullPath((Join-Path -Path $PSScriptRoot -ChildPath "../../Workspace/shared"))

# Ensure shared android folder and cmdline-tools/latest exist by downloading Google's command-line tools
$androidShared = Join-Path -Path $sharedFolder -ChildPath "android"
$cmdlineParent = Join-Path -Path $androidShared -ChildPath "cmdline-tools"
$cmdlineLatest = Join-Path -Path $cmdlineParent -ChildPath "latest"
$jdkTarget = Join-Path -Path $androidShared -ChildPath "jdk17"

# Ensure the shared android root exists and set ANDROID_HOME for this session so subsequent scripts find cmdline-tools
if (-not (Test-Path $androidShared)) {
    New-Item -ItemType Directory -Path $androidShared -Force | Out-Null
}
$env:ANDROID_HOME = [System.IO.Path]::GetFullPath($androidShared)
Write-Host "Set ANDROID_HOME for this session to '$env:ANDROID_HOME'" -ForegroundColor Green
function Get-PlatformInfo {

    $platform = if ($IsWindows) { 'win' } elseif ($IsMacOS) { 'mac' } else { 'linux' }
    $jdkPlatform = if ($IsWindows) { 'windows' } elseif ($IsMacOS) { 'mac' } else { 'linux' }
    $jdkExt = if ($IsWindows) { 'zip' } else { 'tar.gz' }
    return [PSCustomObject]@{ IsWindows = $IsWindows; IsMac = $IsMacOS; Platform = $platform; JdkPlatform = $jdkPlatform; JdkExt = $jdkExt }
}

function Invoke-DownloadAndExtract {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $fileName = [System.IO.Path]::GetFileName($Url)
    if ([string]::IsNullOrEmpty($fileName)) { $fileName = "download" }
    $archivePath = Join-Path -Path $Destination -ChildPath $fileName

    $maxTries = 3
    $try = 0
    $downloaded = Test-Path $archivePath
    while (-not $downloaded -and $try -lt $maxTries) {
        $try++
        try {
            Write-Host "Downloading $Url (attempt $try/$maxTries) ..."
            Invoke-WebRequest -Uri $Url -OutFile $archivePath -UseBasicParsing -ErrorAction Stop
            $downloaded = $true
        }
        catch {
            Write-Host "Download attempt $try failed: $_"
            if ($try -ge $maxTries) {
                Remove-Item -LiteralPath $archivePath -Recurse -Force -ErrorAction SilentlyContinue
                throw "Failed to download $Url after $maxTries attempts."
            }
            Start-Sleep -Seconds (2 * $try)
        }
    }

    if ($archivePath -match '\.zip$') {
        try {
            Expand-Archive -Path $archivePath -DestinationPath $Destination -Force
        }
        catch {
            $err = $_ | Out-String
            Remove-Item -LiteralPath $archivePath -Recurse -Force -ErrorAction SilentlyContinue
            throw "Failed to extract zip $archivePath`n$err"
        }
    }
    elseif ($archivePath -match '\.(tar\.gz|tgz)$') {
        $tar = "tar"
        try {
            Start-Process -FilePath $tar -ArgumentList "-xzf", $archivePath, "-C", $Destination -NoNewWindow -Wait -PassThru -ErrorAction Stop
        }
        catch {
            Remove-Item -LiteralPath $archivePath -Recurse -Force -ErrorAction SilentlyContinue
            throw "Failed to extract tar.gz archive $archivePath : $_"
        }
    }
    else {
        Remove-Item -LiteralPath $archivePath -Recurse -Force -ErrorAction SilentlyContinue
        throw "Unsupported archive format for $archivePath"
    }
}

# Download and install cmdline-tools if missing
if (-not (Test-Path $cmdlineLatest)) {
    Write-Host "cmdline-tools not found at '$cmdlineLatest'. Downloading Android command-line tools..." -ForegroundColor Green

    # determine platform name used by Google's downloads
    $plat = Get-PlatformInfo
    $cmdUrl = "https://dl.google.com/android/repository/commandlinetools-$($plat.Platform)-13114758_latest.zip"

    # create parent
    if (-not (Test-Path $cmdlineParent)) {
        New-Item -ItemType Directory -Path $cmdlineParent -Force | Out-Null
    }

    Invoke-DownloadAndExtract -Url $cmdUrl -Destination $cmdlineParent
    # move extracted 'cmdline-tools' folder to 'latest'
    $extractedCmdline = Join-Path -Path $cmdlineParent -ChildPath "cmdline-tools"
    if (-not (Test-Path $extractedCmdline)) {
        throw "Expected extracted cmdline-tools folder not found at '$extractedCmdline'"
    }
    Move-Item -Path $extractedCmdline -Destination $cmdlineLatest

    # ensure executables are runnable on non-windows
    if (-not $plat.IsWindows) {
        if (Test-Path (Join-Path -Path $cmdlineLatest -ChildPath "bin")) {
            Get-ChildItem -Path (Join-Path -Path $cmdlineLatest -ChildPath "bin") -Recurse -File | ForEach-Object { try { & chmod +x $_.FullName } catch {} }
        }
    }
    Write-Host "Android command-line tools installed to '$cmdlineLatest'." -ForegroundColor Green
}
else {
    Write-Host "cmdline-tools already present at '$cmdlineLatest'. Skipping download."
}

# Download JDK (Eclipse Temurin 17 by default) and set JAVA_HOME for this session

if (-not (Test-Path $jdkTarget)) {
    Write-Host "JDK not found at '$jdkTarget'. Downloading JDK..." -ForegroundColor Green

    $plat = Get-PlatformInfo
    $jdkPlatform = $plat.JdkPlatform
    $jdkExt = $plat.JdkExt

    # Allow overriding via env var JDK_URL
    if ($env:JDK_URL) {
        $jdkUrl = $env:JDK_URL
    }
    else {
        # default to Temurin 17 release asset name pattern
        $jdkUrl = "https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.6%2B10/OpenJDK17U-jdk_x64_${jdkPlatform}_hotspot_17.0.6_10.${jdkExt}"
    }

    $tmpJdkDir = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath "ez_jdk_install"
    if (Test-Path $tmpJdkDir) { Remove-Item -LiteralPath $tmpJdkDir -Recurse -Force -ErrorAction SilentlyContinue }
    New-Item -ItemType Directory -Path $tmpJdkDir | Out-Null

    Invoke-DownloadAndExtract -Url $jdkUrl -Destination $tmpJdkDir

    # Find the top-level folder that looks like a JDK (contains bin/java or bin/java.exe)
    $jdkBinRel = "bin/java"
    $jdkBinWinRel = "bin/java.exe"
    $foundJdk = Get-ChildItem -Path $tmpJdkDir -Directory -ErrorAction SilentlyContinue | Where-Object {
        (Test-Path (Join-Path -Path $_.FullName -ChildPath $jdkBinRel)) -or (Test-Path (Join-Path -Path $_.FullName -ChildPath $jdkBinWinRel))
    } | Select-Object -First 1

    if ($null -eq $foundJdk) {
        # maybe the extract placed bin at top-level
        if ((Test-Path (Join-Path -Path $tmpJdkDir -ChildPath $jdkBinRel)) -or (Test-Path (Join-Path -Path $tmpJdkDir -ChildPath $jdkBinWinRel))) {
            $foundJdk = Get-Item -Path $tmpJdkDir
        }
    }

    if ($null -eq $foundJdk) {
        Remove-Item -LiteralPath $tmpJdkDir -Recurse -Force -ErrorAction SilentlyContinue
        throw "Could not locate JDK installation in the extracted archive."
    }

    # Move/copy found JDK into $jdkTarget
    if (Test-Path $jdkTarget) { Remove-Item -LiteralPath $jdkTarget -Recurse -Force -ErrorAction SilentlyContinue }
    New-Item -ItemType Directory -Path $jdkTarget | Out-Null
    Get-ChildItem -Path $foundJdk.FullName | ForEach-Object {
        $src = $_.FullName
        $dest = Join-Path -Path $jdkTarget -ChildPath $_.Name
        Copy-Item -Path $src -Destination $dest -Recurse -Force
    }

    # make binaries executable on non-windows
    if (-not $plat.IsWindows) {
        if (Test-Path (Join-Path -Path $jdkTarget -ChildPath "bin")) {
            Get-ChildItem -Path (Join-Path -Path $jdkTarget -ChildPath "bin") -Recurse -File | ForEach-Object { try { & chmod +x $_.FullName } catch {} }
        }
    }

    Remove-Item -LiteralPath $tmpJdkDir -Recurse -Force -ErrorAction SilentlyContinue

    Write-Host "JDK installed to '$jdkTarget'." -ForegroundColor Green
}
else {
    Write-Host "JDK already present at '$jdkTarget'. Skipping download."
}

# Set JAVA_HOME for this session
$env:JAVA_HOME = [System.IO.Path]::GetFullPath($jdkTarget)
Write-Host "Set JAVA_HOME for this session to '$env:JAVA_HOME'" -ForegroundColor Green


$sdkManager = Get-SdkManager
if ($acceptLicence) {
    
    for ($i = 0; $i -lt 8; $i++) {
        "y`ny`ny`n" | & $sdkManager --licenses | Out-Null
    }
}

# build dependencies
Install-SdkBuildDependencies -SdkManager $sdkManager

if ($installEmulator) {
    # Emulator packages
    Install-SdkEmulatorPackages -SdkManager $sdkManager
}

Write-Host "Android Licenses..." -ForegroundColor Green
if ($acceptLicence) {
    
    for ($i = 0; $i -lt 8; $i++) {
        "y`ny`ny`n" | & $sdkManager --licenses | Out-Null
    }
}
else {
    & $sdkManager --licenses
}
