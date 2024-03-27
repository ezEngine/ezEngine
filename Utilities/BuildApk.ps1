param(
	[Parameter(Mandatory=$true)]
	[string]$BuildToolsPath,
	
	[Parameter(Mandatory=$true)]
	[string]$ContentDirectory,
	
	[Parameter(Mandatory=$true)]
	[string]$Manifest,
	
	[Parameter(Mandatory=$true)]
	[string]$AndroidPlatformRoot,	
	
	[Parameter(Mandatory=$true)]
	[string]$TargetName,
	
	[Parameter(Mandatory=$true)]
	[string]$OutDir,
	
	[Parameter(Mandatory=$true)]
	[string]$SignKey,
	
	[Parameter(Mandatory=$true)]
	[string]$SignPassword
)

$ErrorActionPreference = "Stop"

$aapt = "$BuildToolsPath/aapt.exe"
$apksigner = "$BuildToolsPath/apksigner.bat"
$zipalign = "$BuildToolsPath/zipalign.exe"
if ($IsLinux) {
    $aapt = "$BuildToolsPath/aapt"
    $apksigner = "$BuildToolsPath/apksigner"
    $zipalign = "$BuildToolsPath/zipalign"
}

if(-not (Test-Path $aapt))
{
	Write-Host "Failed to find aapt.exe. Expected location $aapt."
	exit 1
}

if(-not (Test-path $apksigner))
{
	Write-Host "Failed to find apksigner.bat. Expected location $apksigner."
	exit 1
}

if(-not (Test-Path $zipalign))
{
	Write-Host "Failed to find zipalign.exe. Expected location $zipalign."
	exit 1
}

$unalignedApkPath = "$OutDir/$TargetName.unaligned.apk"
$finalApkPath = "$OutDir/$TargetName.apk"

Write-Host "Building unaligned apk $unalignedApkPath ..."
& $aapt package --debug-mode -f -M $Manifest -S "$ContentDirectory/res" -I "$AndroidPlatformRoot/android.jar" -F $unalignedApkPath $ContentDirectory
if($lastexitcode -ne 0)
{
	exit $lastexitcode
}

Write-Host "Aligning apk $unalignedApkPath -> $finalApkPath ..."
& $zipalign -f 4 $unalignedApkPath $finalApkPath
if($lastexitcode -ne 0)
{
	exit $lastexitcode
}

Write-Host "Signing apk $finalApkPath with key $SignKey ..."
Write-Host "apksigner:`"$apksigner`", SignKey:`"$SignKey`", SignPassword: `"$SignPassword`""

& $apksigner sign --ks $SignKey --ks-pass $SignPassword $finalApkPath
if($lastexitcode -ne 0)
{
    Write-Host "FAILED with $lastexitcode"
	exit $lastexitcode
}
Write-Host "Done building apk $finalApkPath"