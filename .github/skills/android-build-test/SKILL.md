---
name: android-build-test
description: Build and test ezEngine on Android devices. Use this skill when building APKs, deploying to Android devices, running tests on Android, debugging Android test failures, or setting up the Android development environment for ezEngine.
---

# Building and Testing ezEngine on Android

This skill covers the full Android workflow: environment setup, CMake configuration, building APKs, deploying to devices, and running tests.

## Prerequisites

### Installing Android SDK/NDK Dependencies

Run the dependency installer once to download the Android SDK, NDK, build-tools, and JDK into `Workspace/shared/android/`:

```bash
pwsh ./Utilities/Android/InstallAndroidDependencies.ps1 -acceptLicence
```

This downloads and configures:
- Android SDK command-line tools
- Android NDK 26.1.10909125
- Android build-tools 34.0.0
- JDK 17
- Android platform API 29

### Setting Environment Variables

Before running any Android commands in the terminal, source the environment setup script:

**Bash:**
```bash
source Utilities/Android/SetupAndroidEnvVars.sh
```

**PowerShell:**
```powershell
. ./Utilities/Android/SetupAndroidEnvVars.ps1
```

This exports `ANDROID_HOME`, `ANDROID_NDK_HOME`, `JAVA_HOME` and adds `platform-tools` (adb) to `PATH`.

The CMake presets also set these variables internally, so they are only needed for manual adb/device interaction.

## CMake Presets

All Android presets inherit from the hidden `android-base` preset which configures the NDK toolchain, Ninja generator, and Vulkan support.

| Preset | ABI | Build Type |
|---|---|---|
| `android-arm64-debug` | arm64-v8a | Debug |
| `android-arm64-dev` | arm64-v8a | Dev |
| `android-arm64-shipping` | arm64-v8a | Shipping |
| `android-x64-debug` | x86_64 | Debug |
| `android-x64-dev` | x86_64 | Dev |
| `android-x64-shipping` | x86_64 | Shipping |

Use `arm64` presets for physical devices and `x64` presets for x86_64 emulators.

## Building

### Configure and Build Everything

```bash
cmake --preset android-arm64-debug
cmake --build Workspace/android-arm64-debug
```

### Build a Specific Target

```bash
cmake --build Workspace/android-arm64-debug --target FoundationTest
```

Build outputs (APKs and shared libraries) go to: `Output/Bin/AndroidNinjaClangDebugArm64/`

The output directory name follows the pattern `AndroidNinjaClang<BuildType>Arm64` (or `X64` for x86_64).

| Preset | Output Directory |
|---|---|
| `android-arm64-debug` | `Output/Bin/AndroidNinjaClangDebugArm64/` |
| `android-arm64-dev` | `Output/Bin/AndroidNinjaClangDevArm64/` |
| `android-x64-debug` | `Output/Bin/AndroidNinjaClangDebugX64/` |

### Available Test Targets

- `FoundationTest` — Foundation library tests (math, strings, containers, IO, threading, etc.)
- `CoreTest` — Core library tests (resource system, world, etc.)
- `RendererTest` — Renderer tests (requires Vulkan on device)
- `ToolsFoundationTest` — Tools foundation tests

Each target produces an APK at `Output/Bin/<OutputDir>/<TargetName>.apk`.

## Connecting to a Device

### Physical Device via ADB over TCP/IP

```bash
source Utilities/Android/SetupAndroidEnvVars.sh
adb connect <device-ip>:5555
adb -s <device-ip>:5555 wait-for-device
```

### Verify Connection

```bash
adb devices
```

## Running Tests on Device

### Using AndroidTest.ps1 (Recommended)

The `Utilities/Android/AndroidTest.ps1` script handles the full test lifecycle: install APK, launch activity, capture logcat, detect pass/fail, download artifacts.

**Important:** You must `source` the environment setup script first so that `adb` is on `PATH`. Running `pwsh` from bash inside VS Code's integrated terminal can crash the terminal session when the script exits. To avoid this, run the command in a standalone terminal outside VS Code, or launch it as a background process.

```bash
source Utilities/Android/SetupAndroidEnvVars.sh
pwsh ./Utilities/Android/AndroidTest.ps1 \
  -deviceAdb <device-ip>:5555 \
  -packageName com.ezengine.FoundationTest \
  -activityName android.app.NativeActivity \
  -outputFolder ./Output \
  -apk ./Output/Bin/AndroidNinjaClangDebugArm64/FoundationTest.apk
```

#### Passing Command-Line Arguments

Use `-arguments` to pass the same flags you would use on desktop (e.g., `-run`, `-noGui`, `-all`, `-filter`):

```bash
pwsh ./Utilities/Android/AndroidTest.ps1 \
  -deviceAdb <device-ip>:5555 \
  -packageName com.ezengine.FoundationTest \
  -activityName android.app.NativeActivity \
  -outputFolder ./Output \
  -apk ./Output/Bin/AndroidNinjaClangDebugArm64/FoundationTest.apk \
  -arguments "-run -noGui -filter Frustum"
```

Note: `-all` and `-filter` are mutually exclusive. Use `-filter <regex>` to run a subset of tests.

The arguments are delivered to the native code via an Android Intent string extra (`args`), retrieved via JNI in `AndroidTestApplication.cpp`, parsed into argc/argv, and forwarded to `InitTestFramework`.

#### AndroidTest.ps1 Parameters

| Parameter | Required | Description |
|---|---|---|
| `-deviceAdb` | Yes | Device address for adb (e.g., `192.168.178.77:5555`) |
| `-packageName` | Yes | Android package name (e.g., `com.ezengine.FoundationTest`) |
| `-activityName` | Yes | Activity class name (always `android.app.NativeActivity`) |
| `-outputFolder` | Yes | Local directory for logcat output and test artifacts |
| `-apk` | No | Path to APK to install before running |
| `-arguments` | No | Command-line arguments to pass to the test framework |
| `-MessageBoxOnError` | No | Show error dialog on Windows (switch) |

### Test Package Names

| Test | Package Name |
|---|---|
| FoundationTest | `com.ezengine.FoundationTest` |
| CoreTest | `com.ezengine.CoreTest` |
| RendererTest | `com.ezengine.RendererTest` |
| ToolsFoundationTest | `com.ezengine.ToolsFoundationTest` |

### Running All Four Test Suites

```bash
DEVICE=192.168.178.77:5555
OUTDIR=Output/Bin/AndroidNinjaClangDebugArm64

for TEST in FoundationTest CoreTest RendererTest ToolsFoundationTest; do
  pwsh ./Utilities/Android/AndroidTest.ps1 \
    -deviceAdb $DEVICE \
    -packageName com.ezengine.$TEST \
    -activityName android.app.NativeActivity \
    -outputFolder ./Output \
    -apk ./$OUTDIR/$TEST.apk
done
```

### Manual Testing via ADB

If you need to launch a test manually without the script:

```bash
source Utilities/Android/SetupAndroidEnvVars.sh

# Install
adb -s <device>:5555 install -r -t Output/Bin/AndroidNinjaClangDebugArm64/FoundationTest.apk

# Launch without arguments
adb -s <device>:5555 shell am start -n com.ezengine.FoundationTest/android.app.NativeActivity

# Launch with arguments (note the quoting)
adb -s <device>:5555 shell "am start -n com.ezengine.FoundationTest/android.app.NativeActivity --es args '-run -noGui -filter Frustum'"

# Watch logcat
adb -s <device>:5555 logcat -s ezEngine
```

Important: When passing arguments containing dashes via `adb shell am start --es`, you **must** wrap the entire `am start` command in double quotes and the argument value in single quotes. Otherwise, `am` will misinterpret the dashes as its own flags.

## Shader Compilation

Android uses Vulkan shaders that must be precompiled on the host. The CI pipeline builds a Linux ShaderCompiler first, then runs:

```bash
pwsh ./Utilities/Android/CompileShaders.ps1 -BinariesDir ./Output/Bin/LinuxNinjaClangDev64
```

This compiles shaders for `Data/Base`, `Data/UnitTests`, and `Data/Samples/ShaderExplorer`. You need a working Linux build of `ShaderCompiler` before compiling shaders.

## Debugging

### Reading Logcat

All ezEngine log output uses the tag `ezEngine`:

```bash
# Live logcat filtered to ezEngine
adb -s <device>:5555 logcat -s ezEngine

# Dump existing logcat
adb -s <device>:5555 logcat -d -s ezEngine

# Clear logcat before a test run
adb -s <device>:5555 logcat --clear
```

### LLDB Remote Debugging

Use the provided debug script:

```bash
pwsh ./Utilities/Android/DbgAndroidLldb.ps1
```

### Common Issues

**Test runs all tests despite `-filter`**: The arguments must survive multiple quoting layers (PowerShell → adb → Android shell → `am`). When using `AndroidTest.ps1`, pass arguments as a single string: `-arguments "-filter Frustum"`. When using `adb` directly, wrap the whole `am start` command: `adb shell "am start ... --es args '-filter Frustum'"`.

**APK install fails**: Ensure the device is connected (`adb devices`), USB debugging is enabled, and the device allows unknown sources. The script retries up to 6 times.

**Activity crashes immediately**: Check logcat for the full stack trace. Common causes include missing Vulkan drivers (for RendererTest) or missing shader cache files.

**Build fails with NDK not found**: Ensure dependencies are installed (`InstallAndroidDependencies.ps1`) and environment variables are set. The CMake presets expect the NDK at `Workspace/shared/android/ndk/26.1.10909125`.

## CI Pipeline Reference

The CI pipeline in `Code/BuildSystem/AzurePipelines/Android-arm64.yml` performs the following steps:

1. Configure and build `linux-clang-dev` (host tools only, specifically `ShaderCompiler`)
2. Compile Vulkan shaders using the host ShaderCompiler
3. Install Android dependencies
4. Configure and build `android-arm64-dev`
5. Connect to the physical test device
6. Run FoundationTest, CoreTest, RendererTest, ToolsFoundationTest sequentially
7. Disconnect from device and publish test artifacts

## Key Source Files

- `Utilities/Android/AndroidTest.ps1` — Test runner script (install, launch, logcat, pass/fail detection)
- `Utilities/Android/AndroidUtils.ps1` — Shared utilities (adb wrappers, retry logic, file copy)
- `Utilities/Android/InstallAndroidDependencies.ps1` — SDK/NDK/JDK dependency installer
- `Utilities/Android/CompileShaders.ps1` — Vulkan shader compiler wrapper
- `Utilities/Android/SetupAndroidEnvVars.sh` — Bash environment variable setup
- `Utilities/Android/SetupAndroidEnvVars.ps1` — PowerShell environment variable setup
- `Utilities/Android/BuildApk.ps1` — APK packaging script (called by CMake)
- `Code/UnitTests/TestFramework/Platform/Android/AndroidTestApplication.cpp` — Native activity lifecycle and Intent argument retrieval
- `Code/UnitTests/TestFramework/Platform/Android/TestFrameworkEntryPoint_Platform.h` — Android test entry point macro
- `Code/Engine/Foundation/Platform/Android/Utils/AndroidJni.h` — JNI wrapper classes (ezJniAttachment, ezJniObject, ezJniString)
- `CMakePresets.json` — Android CMake preset definitions
