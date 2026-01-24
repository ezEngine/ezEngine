#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <d3d11_1.h>
// Enable D3D11 OpenXR types
#  define XR_USE_GRAPHICS_API_D3D11
#endif

// Vulkan support - include vulkan.h if EZ_OPENXR_HAS_VULKAN_RENDERER is enabled
// EZ_OPENXR_HAS_VULKAN_RENDERER is defined by CMake when RendererVulkan is available
#if EZ_OPENXR_HAS_VULKAN_RENDERER
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
#endif // EZ_OPENXR_HAS_VULKAN_RENDERER

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <jni.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>

// OpenXR remoting support removed.
