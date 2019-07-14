@ECHO OFF

SET LOCAL_VCPKG_ROOT=%VCPKG_ROOT%

IF "%VCPKG_ROOT%" == "" (
  ECHO VCPKG_ROOT environment variable is not set, using '%~dp0vcpkg'
  ECHO You may need to reboot Windows for this change to propagate to other applications, such as CMake.

  SETX VCPKG_ROOT %~dp0vcpkg
  SET LOCAL_VCPKG_ROOT=%~dp0vcpkg
)

IF NOT EXIST "%LOCAL_VCPKG_ROOT%\README.md" (
  ECHO Installing vcpkg to '%LOCAL_VCPKG_ROOT%'

  git clone https://github.com/Microsoft/vcpkg.git "%LOCAL_VCPKG_ROOT%"
)

IF NOT EXIST "%LOCAL_VCPKG_ROOT%\README.md" (
  ECHO FATAL ERROR: Could not clone vcpkg. Ensure that git is installed and accessible.
  exit /b 1
)

IF NOT EXIST "%LOCAL_VCPKG_ROOT%\vcpkg.exe" (
  ECHO Bootstrapping vcpkg in '%LOCAL_VCPKG_ROOT%'
  CALL %LOCAL_VCPKG_ROOT%\bootstrap-vcpkg.bat
)

IF NOT EXIST "%LOCAL_VCPKG_ROOT%\vcpkg.exe" (
  ECHO FATAL ERROR: Bootstrapping vcpkg failed
  exit /b 2
)

ECHO Finished installing vcpkg to '%LOCAL_VCPKG_ROOT%'