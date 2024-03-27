param(
	[switch]$MessageBoxOnError
)

function RaiseError {
	param(
		[string]$msg
	)
	if ($MessageBoxOnError -and $IsWindows) {
		[System.Windows.Forms.MessageBox]::Show($msg)
	}
	else {
		Write-Host $msg -foreground red
	}
	exit 1
}

$jdb = "$env:JAVA_HOME/bin/jdb"
if ($IsWindows) {
	$jdb = "$jdb.exe"
}
if (-not (Test-Path $jdb)) {
	RaiseError "Failed to find jdb executable in $jdb. Please ensure that the JAVA_HOME environment variable is correctly set."
}
$jdb = Resolve-Path $jdb

if ($IsWindows) {
	Start-Process -FilePath "$env:comspec" -ArgumentList "/C `"`"$jdb`" -connect com.sun.jdi.SocketAttach:port=12345,hostname=localhost`"" -WindowStyle Hidden
}
else {
	Start-Process -FilePath "$jdb" -ArgumentList "-attach localhost:12345" -PassThru
}	
	

