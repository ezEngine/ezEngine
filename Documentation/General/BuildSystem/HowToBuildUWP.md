How to build for UWP {#HowToBuildUWP}
================

Using the command line
----------------
Run CMake with the WindowsStore toolchain file found in the BuildSystem/CMake folder:
**-DCMAKE_TOOLCHAIN_FILE**=_<RepositoryDirectory>/Code/BuildSystem/CMake/toolchain-winstore.cmake_

Using the Windows-UI
----------------
* Specify your target Visual Studio compiler.
* Check "Specify toolchain file for cross-compiling".
* On the next screen set the WindowsStore toolchain file found in the folder Code/BuildSystem/CMake:
  <RepositoryDirectory>/Code/BuildSystem/CMake/toolchain-winstore.cmake
