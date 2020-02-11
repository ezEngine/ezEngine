How to build for Android {#HowToBuildAndroid}
================

Prerequisites
-------------

You will need to following to build for Android:
  * Android SDK Platform 6.0 (Marshmallow) API-Level 23
  * Android SDK Build-Tools
  * Android NDK 20 or higher
  * Android SDK Tools
  * Android SDK Platform-Tools
  * Java (JRE)
  * Ninja
  * Android Emulator (optional)

[Ninja](https://ninja-build.org/) is a build generator used by CMake and needs to be added to the **PATH** environment variable.\
The easiest way to install the Android components is to download [Android Studio](https://developer.android.com/studio) and then to select these from the **SDK Manager**.
Once installed, the following environment variables need to be set:
  * **ANDROID_BUILD_TOOLS** needs to point to your installed version, by default this is: `C:\Users\[USERNAME]\AppData\Local\Android\Sdk\build-tools\29.0.2\`
  Change the version to reflect the one you are using.
  * **ANDROID_NDK** needs to point to your installed version, by default this is: `C:\Users\[USERNAME]\AppData\Local\Android\Sdk\ndk-bundle`
  * **ANDROID_SDK** needs to point to your installed version, by default this is: `C:\Users\[USERNAME]\AppData\Local\Android\Sdk`
  * **JAVA_HOME** needs to point to a java runtime. Android Studio has its own version so there is no need to download it separately: `C:\Program Files\Android\Android Studio\jre`


Visual Studio Open Folder
-------------------------
While you can manually run cmake to use the ninja generator, a more convenient solution is to use Visual Studio's open folder functionality. Go to `File>Open>Folder...` and select the root of the repository. The cmake settings used by this feature are stored inside the `CMakeSettings.json` file in the root folder (`CppProperties.json` contains additional information for VS). Note that if a different API level is used, the **ANDROID_PLATFORM** parameter in the `CMakeSettings.json` has to be changed for all configurations.

If all environment variables were set correctly VS should automatically configure CMake. Once done, a drop down appears in the VS toolbar, allowing you to select the configuration, e.g. `Android-x86-Debug`. Once changed, VS will start to configure Cmake again for the new configuration. 

Next, select a build target, e.g. `libFoundationTest.so` which are the foundation unit tests. Note that you can only select applications, not all libraries here.

Setting up an Emulator
----------------------
Open Android Studio, go to `Configure>AVD Manager` and select `Create Virtual Device`. Download the **Pixel 2 x86** image (or any other compatible one). Next, select **Pie (API 28)** (29 is broken as of writing). In `Advanced Settings` go to `Emulated Performance` and select **Cold boot** as the other options tend to hang and the only option then is to reset the image to factory defaults.

Debugging Code
--------------
This approach requires Android development for VS to be installed. In VS, go to `Tools>Options..` then ` Cross Platform>C++>Android` and change Android SDK and Android NDK paths to those matching the environment variables.

Then, follow the steps above to select your target application. Let's assume `FoundationTest` should be run in **debug** on the emulator. First it needs to be build via `Build>Build libFoundationTest.so`. Once succeeded, an apk is found in in `Output\Lib\AndroidNinjaClangDebug32\FoundationTest.apk`.

Open another instace of VS and select `File>Open>Project/Solution` and open the apk file. Switch to **x86** and select the emulator device next to it.\
RMB on the project and select `Properties`. Change **Additional Symbol Search Paths** to point to `Output\Lib\AndroidNinjaClangDebug32`.

**Warning!** Any **Android Studio** window must be closed before pressing **Start Debugging** as it will prevent **adb** from working. Once that's done debugging can be done in VS like any other debugging target.

VS has a build-in logcat window but the filter is limited so it is better to use the following commandline to only show ezEngine logs:
```
%ANDROID_SDK%\platform-tools\adb.exe logcat ezEngine:D *:S
```

