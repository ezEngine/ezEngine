How to build with Clang on Windows {#HowToBuildClangOnWindows}
================

You can build the engine using the Clang front-end on Windows through Visual Studio.
This can be useful to debug difficult Clang errors. However, at this point, Clang is not able to properly compile the engine on Windows, so you will need to ignore several errors.

Using the CMake Windows-UI
----------------
* Specify your target Visual Studio compiler.
* In the field _Optional toolset to use (-T parameter)_ type **v140\_clang\_c2**
* Configure
* In the settings, disable EZ\_USE\_PCH
* Generate