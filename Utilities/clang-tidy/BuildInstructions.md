# How to build the ezEngine custom version of clang-tidy

All commands given should be executed in a powershell.

 * get the llvm source: `git clone https://github.com/llvm/llvm-project`
 * `cd llvm-project`
 * checkout the latest release version (for the current build llvm-15.0.3 is used): `git checkout llvmorg-15.0.3`
 * Apply the llvm.patch: `git apply llvm.patch`
 * Copy the ez folder to `llvm-project/clang-tools-extra/clang-tidy/ez`
 * create a build folder `mkdir build`
 * `cd build`
 * Run cmake: `cmake -B . -S ..\llvm -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang" -DLLVM_TARGETS_TO_BUILD=X86`
 * Open the generated solution and build the clang-tidy executable in Release. Copy the resulting executable to `ezEngine/Data/Tools/Precompiled/clang-tidy/clang-tidy.exe` when done
