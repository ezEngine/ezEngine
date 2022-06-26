#pragma once

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

struct ezStagingBufferVulkan
{
  vk::Buffer m_buffer;
  ezVulkanAllocation m_alloc;
  ezVulkanAllocationInfo m_allocInfo;
};

class EZ_RENDERERVULKAN_DLL ezStagingBufferPoolVulkan
{
public:
  void Initialize(ezGALDeviceVulkan* pDevice);
  void DeInitialize();

  ezStagingBufferVulkan AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size);
  void ReclaimBuffer(ezStagingBufferVulkan& buffer);

private:
  ezGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
};
