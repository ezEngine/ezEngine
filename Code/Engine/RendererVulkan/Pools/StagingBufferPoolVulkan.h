#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#include <vulkan/vulkan.hpp>

struct ezStagingBufferVulkan
{
  vk::Buffer m_buffer;
  ezVulkanAllocation m_alloc;
  ezVulkanAllocationInfo m_allocInfo;
};

class EZ_RENDERERVULKAN_DLL ezStagingBufferPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static ezStagingBufferVulkan AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size);
  static void ReclaimBuffer(ezStagingBufferVulkan& buffer);

private:
  static vk::Device s_device;
};
