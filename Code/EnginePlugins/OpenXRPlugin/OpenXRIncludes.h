#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <d3d11_1.h>
// Enable D3D11 OpenXR types
#  define XR_USE_GRAPHICS_API_D3D11
#endif

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    define VK_USE_PLATFORM_WIN32_KHR
#  elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#    define VK_USE_PLATFORM_XCB_KHR
#  elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
#    define VK_USE_PLATFORM_ANDROID_KHR
#  endif
#  include <vulkan/vulkan.h>
// Enable Vulkan OpenXR types
#  define XR_USE_GRAPHICS_API_VULKAN
#endif

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <jni.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>

