# How to build the ezEngine custom version of clang-tidy

All commands given should be executed in a powershell.

 * get the llvm source: `git clone https://github.com/llvm/llvm-project`
 * `cd llvm-project`
 * checkout the latest release version (for the current build llvm-18.1.7 is used): `git checkout llvmorg-18.1.7`
 * Apply the llvm.patch: `git apply llvm.patch`
 * Copy the ez folder to `llvm-project/clang-tools-extra/clang-tidy/ez`
 * create a build folder `mkdir build`
 * `cd build`
 * Run cmake: `cmake -B . -S ..\llvm -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang" -DLLVM_TARGETS_TO_BUILD=X86`
 * Open the generated solution and build the clang-tidy executable (Clang executables folder) in Release.
 * Copy the resulting executable from `build/Release/bin/clang-tidy.exe` to `ezEngine/Data/Tools/Precompiled/clang-tidy/clang-tidy.exe` when done

# How to run clang-tidy locally

* Run `Utilities\clang-tidy\SetupWorkspace.ps1`. This will download llvm and create a compatible cmake project that clang-tidy can work on. It is not advised to compile the entirety of llvm yourself, so it is best only compile clang-tidy and download pre-built binaries for everything else.
* If you have made modifications to `clang-tidy.exe` copy it into the `Data\Tools\Precompiled\clang-tidy` folder.
  * If you just want to check a single file, run `Utilities\clang-tidy\RunClangTidy.ps1 -SingleFile ABS_PATH_TO_FILE`.
  * If you want to check all your changes against `dev`, first commit them, then run:\
  `Utilities\clang-tidy\RunClangTidy.ps1`

## Debugging clang-tidy

⚠️When running clang-tidy.exe out of the directory it was build into, it will pick up a different set of header files and compile errors might appear. So copy the executable out of the build directory before attempting to run it on the ezEngine source code. For local minimal test cases the clang-tidy.exe can remain in the build output directory.\
If you need to run and debug on EZ code, open the clang-tidy VS properties and do the following.
  * Change the paths in the steps below as follows:
    * **C:\Code\llvm-project**: The root directory of your llvm-project clone.
    * **C:\Code\ezEngine**: The root directory of your EZ clone.
    * **C:\Code\ezEngine\Code\Engine\Core\ResourceManager\Implementation\Resource.cpp**: The file you want to check.
  * Build Events -> Post-Build-Event: `copy C:\Code\llvm-project\build\Debug\bin\clang-tidy.exe "C:\Code\ezEngine\Data\Tools\Precompiled\clang-tidy\clang-tidy.exe" /Y`
  * Debugging:
    * Command: `C:\Code\ezEngine\Data\Tools\Precompiled\clang-tidy\clang-tidy.exe`
    * Command Arguments: `-p C:\Code\ezEngine\Workspace\clang-tidy --checks=-*,ez-name-check,modernize-use-default-member-init,modernize-use-equals-default,modernize-use-using,clang-analyzer-core.* "--header-filter=^((?!ThirdParty|DirectXTex|ogt_vox|ui_).)*$" --extra-arg=-DBUILDSYSTEM_CLANG_TIDY=1 "--extra-arg=-isystemC:\Code\ezEngine\llvm\lib\clang\16\include" C:\Code\ezEngine\Code\Engine\Core\ResourceManager\Implementation\Resource.cpp`
    * Working Directory: `C:\Code\ezEngine\Data\Tools\Precompiled\clang-tidy`