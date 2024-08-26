set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_DEBUG "Debug")
set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_DEV "Dev")
set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_RELEASE "Shipping")

set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_DEBUG_UPPER "DEBUG")
set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_DEV_UPPER "DEV")
set_property(GLOBAL PROPERTY EZ_BUILDTYPENAME_RELEASE_UPPER "SHIPPING")

set_property(GLOBAL PROPERTY EZ_DEV_BUILD_LINKERFLAGS "DEBUG")

set_property(GLOBAL PROPERTY EZ_CMAKE_RELPATH "Code/BuildSystem/CMake")
set_property(GLOBAL PROPERTY EZ_CMAKE_RELPATH_CODE "Code")

set_property(GLOBAL PROPERTY EZ_CONFIG_PATH_7ZA "Data/Tools/Precompiled/7z.exe")

set_property(GLOBAL PROPERTY EZ_CONFIG_QT_WINX64_VERSION "Qt6-6.4.0-vs143-x64")
set_property(GLOBAL PROPERTY EZ_CONFIG_QT_WINX64_URL "https://github.com/ezEngine/thirdparty/releases/download/Qt6-6.4.0-vs143-x64/Qt6-6.4.0-vs143-x64.7z")

set_property(GLOBAL PROPERTY EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION "1.3.275.0")
set_property(GLOBAL PROPERTY EZ_CONFIG_VULKAN_SDK_LINUXX64_URL "https://sdk.lunarg.com/sdk/download/1.3.275.0/linux/vulkansdk-linux-x86_64-1.3.275.0.tar.xz")

# Android validation layers version 1.3.275.0 are broken, so a different version is used compared to the SDK.
set_property(GLOBAL PROPERTY EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION "1.3.280.0")
set_property(GLOBAL PROPERTY EZ_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL "https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/vulkan-sdk-1.3.280.0/android-binaries-1.3.280.0.tar.gz")