How to build for MacOS {#HowToBuildMacOS}
================

Prerequisites
-------------

You will need to install the following libraries:
  * XQuartz 2.7.5
  * SFML-2.5.1
  * Qt 5.11 (optional)

A good way to do so is via [homebrew](https://brew.sh/):
```
brew update
brew install Caskroom/cask/xquartz
brew install qt5
brew install sfml
```

Using the command line
----------------
Run CMake with `CMAKE_PREFIX_PATH` pointing to the dependencies listed above. In this example, a `build` folder is created under the root of the repo and cmake is executed in it:
```
cmake -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.13.1/;/usr/local/Cellar/sfml/2.5.1/ -DEZ_ENABLE_QT_SUPPORT=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DEZ_ENABLE_FOLDER_UNITY_FILES=$(unityfiles) -G "Xcode" ../
```
Afterwards the generated solution can be opened in XCode.