function global:Find-EzExe
{
	param([string]$ExeName)

	$VsVersions = @("2026", "2022")
	$Configurations = @("Debug", "Dev", "Shipping")

	# prefer newer version and faster config
	for ($versionIndex=$VsVersions.length-1; $versionIndex -ge 0; $versionIndex--)
	{
		for ($configIndex=$Configurations.length-1; $configIndex -ge 0; $configIndex--)
		{
			$version = $VsVersions[$versionIndex]
			$config = $Configurations[$configIndex]

			$fileToCheck = "$PSScriptRoot\..\..\Output\Bin\WinVs" + $version + $config +"64\" + $ExeName
			if (Test-Path $fileToCheck -PathType leaf)
			{
				return $fileToCheck
			}
		}
	}
}

function global:Find-EditorProcessor
{
	return Find-EzExe "ezEditorProcessor.exe"
}
