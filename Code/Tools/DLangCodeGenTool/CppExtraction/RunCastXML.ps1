# Execute from an admin "Developer PowerShell for VS 2019" command prompt
# castxml.exe must be in the PATH environment variable

set-executionpolicy remotesigned

castxml.exe --castxml-cc-msvc cl  --castxml-output=1 -o "ez.xml" -std=c++17 "CastXmlInput.cpp" -I "$PSScriptRoot/../../../Engine" -I "$PSScriptRoot/../../../EnginePlugins"