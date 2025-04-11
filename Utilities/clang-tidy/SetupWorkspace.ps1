      
      $originalDir = Get-Location
      Set-Location (Join-Path -Path $PSScriptRoot -ChildPath "..\..")
      try {
            if (-Not (Test-Path -Path "$pwd\llvm")) {
                  Write-Host "LLVM not found, downloading LLVM and Ninja..."
                  Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.7/LLVM-18.1.7-win64.exe -OutFile "LLVM-18.1.7-win64.exe"
                  Invoke-WebRequest https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-win.zip -OutFile ninja-win.zip
                  ./Data/Tools/Precompiled/7z x -ollvm LLVM-18.1.7-win64.exe
                  ./Data/Tools/Precompiled/7z x -ollvm ninja-win.zip
            }
      
            $WindowsSdkVersion = ((Get-ChildItem -Directory "C:\Program Files (x86)\Windows Kits\10\bin").Name | ? { $_ -match "^10\.0\." } | sort -Descending)[0]
            Write-Host "Windows SDK Found" $WindowsSdkVersion
            $rcExe = "C:\Program Files (x86)\Windows Kits\10\bin\$WindowsSdkVersion\x64\rc.exe" -replace "\\","/"
            $clangCppExe = "$pwd\llvm\bin\clang++.exe" -replace "\\","/"
            $clangExe = "$pwd\llvm\bin\clang.exe" -replace "\\","/"
            $ninjaExe = "$pwd\llvm\ninja.exe" -replace "\\","/"
            $cmakeCommand = ".\Data\Tools\Precompiled\cmake\bin\cmake.exe -G Ninja -B Workspace/clang-tidy -S . '-DCMAKE_MAKE_PROGRAM=$ninjaExe' '-DCMAKE_CXX_COMPILER=$clangCppExe' '-DCMAKE_C_COMPILER=$clangExe' '-DCMAKE_RC_COMPILER=$rcExe' -DCMAKE_RC_COMPILER_INIT=rc -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DEZ_USE_PCH=OFF -DEZ_ENABLE_FOLDER_UNITY_FILES=OFF '-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion'"
            Write-Host "////////////////////////////////////////////////////////////////////////////////////////////////////////////"
            Write-Host "// CMake Command: $cmakeCommand"
            Write-Host "////////////////////////////////////////////////////////////////////////////////////////////////////////////"
            Invoke-Expression $cmakeCommand
      } finally {
            Set-Location $originalDir
      }
 
    