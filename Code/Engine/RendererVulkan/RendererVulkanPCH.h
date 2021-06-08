#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#define VULKAN_HPP_NO_NODISCARD_WARNINGS // TODO: temporarily disable warnings to make it compile. Need to fix all the warnings later.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
