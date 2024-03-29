#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <vulkan/vulkan_format_traits.hpp>


#if VK_HEADER_VERSION < 268
#  error "Your vulkan headers are to old. The headers of SDK 1.3.211 or newer are required"
#endif

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
