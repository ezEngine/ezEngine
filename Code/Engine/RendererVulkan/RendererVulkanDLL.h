#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define EZ_RENDERERVULKAN_DLL EZ_DECL_EXPORT
#  else
#    define EZ_RENDERERVULKAN_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
//#define VK_LOG_LAYOUT_CHANGES

#define EZ_GAL_VULKAN_RELEASE(vulkanObj) \
  do                                     \
  {                                      \
    if ((vulkanObj) != nullptr)          \
    {                                    \
      (vulkanObj)->Release();            \
      (vulkanObj) = nullptr;             \
    }                                    \
  } while (0)

#define VK_ASSERT_DEBUG(code)                                                                                           \
  do                                                                                                                    \
  {                                                                                                                     \
    auto s = (code);                                                                                                    \
    EZ_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      EZ_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), EZ_SOURCE_FILE, EZ_SOURCE_LINE);            \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                           \
  do                                                                                                                  \
  {                                                                                                                   \
    auto s = (code);                                                                                                  \
    EZ_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      EZ_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), EZ_SOURCE_FILE, EZ_SOURCE_LINE);          \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      ezLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", EZ_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), EZ_SOURCE_FILE, EZ_SOURCE_LINE); \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                    \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      ezLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", EZ_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), EZ_SOURCE_FILE, EZ_SOURCE_LINE); \
      return s;                                                                                                                                                           \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_EZ_FAILURE(code)                                                                                                                             \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      ezLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", EZ_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), EZ_SOURCE_FILE, EZ_SOURCE_LINE); \
      return EZ_FAILURE;                                                                                                                                                  \
    }                                                                                                                                                                     \
  } while (false)
