#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

vk::Device ezStagingBufferPoolVulkan::s_device;

void ezStagingBufferPoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void ezStagingBufferPoolVulkan::DeInitialize()
{
  s_device = nullptr;
}

ezStagingBufferVulkan ezStagingBufferPoolVulkan::AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size)
{
  //#TODO_VULKAN alignment
  ezStagingBufferVulkan buffer;

  EZ_ASSERT_DEBUG(s_device, "ezStagingBufferPoolVulkan::Initialize not called");
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;


  ezVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite;

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, buffer.m_buffer, buffer.m_alloc, &buffer.m_allocInfo));

  return buffer;
}

void ezStagingBufferPoolVulkan::ReclaimBuffer(ezStagingBufferVulkan& buffer)
{
  EZ_ASSERT_DEBUG(s_device, "ezStagingBufferPoolVulkan::Initialize not called");
  ezMemoryAllocatorVulkan::DestroyBuffer(buffer.m_buffer, buffer.m_alloc);
}
