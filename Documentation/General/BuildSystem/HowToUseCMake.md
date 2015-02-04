How to use CMake {#HowToUseCMake}
================

  * Download CMake from [www.cmake.org](http://www.cmake.org) and install it.

  * Run the CMake GUI.

  * For "Where is the source code" select "Trunk".

  * For "Where to build the binaries" select an empty folder, where you want the solution to end up in, for example "Trunk/Workspace", but it can also be somewhere completely different (e.g. another hard drive even).

  * Click "Configure" once.

  * A lot of red should appear (newly found configuration variables). The default values should work fine. Later you can check out what configuration options there are.

  * Click "Configure" again. Now the build will be configured with the settings that you have selected (ie. default values).
    * If still more red stuff appears, that means during the second run more configuration variables were found (probably because you modified something).
    * Just click "Configure" until no red stuff is there anymore.

  * Now click "Generate" -> The solution will be generated in the folder that you specified.

  * Open up the solution, compile and have fun.


Re-run CMake (ie. press "Configure" and "Generate") everytime changes have been done, that are not automatically detected (e.g. you added a file to a folder) -> Visual Studio will usually detect outside changes and reload the project. Usually that works without crashing ;-)

Every once in a while you should clear the CMake cache ("File -> Delete Cache") and regenerate the solution completely, to ensure that no old stuff is still lying around (or just delete the entire workspace folder). This should only be necessary when major changes to the build system have been done.


Using ezEngine with Custom CMake Based Applications {#CMakeFindEZ}
---------------------------------------------------

In "Code\BuildSystem\CMake" you can find the file "FindezEngine.cmake" which makes it easier to integrate ezEngine into a project that already uses CMake.

Further usage details are given inside that file.
