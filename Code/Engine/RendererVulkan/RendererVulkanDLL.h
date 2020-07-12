#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
    #define EZ_RENDERERVULKAN_DLL __declspec(dllexport)
  #else
    #define EZ_RENDERERVULKAN_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_RENDERERVULKAN_DLL
#endif


#define EZ_GAL_VULKAN_RELEASE(vulkanObj) \
  do                                     \
  {                                      \
    if ((vulkanObj) != nullptr)          \
    {                                    \
      (vulkanObj)->Release();            \
      (vulkanObj) = nullptr;             \
    }                                    \
  } while (0)
