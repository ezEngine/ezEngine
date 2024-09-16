#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void ezStagingBufferPoolVulkan::Initialize(ezGALDeviceVulkan* pDevice)
{
  m_pDevice = pDevice;
  m_device = pDevice->GetVulkanDevice();
}

void ezStagingBufferPoolVulkan::DeInitialize()
{
  m_device = nullptr;
}

ezStagingBufferVulkan ezStagingBufferPoolVulkan::AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size)
{
  const vk::PhysicalDeviceProperties& properties = m_pDevice->GetPhysicalDeviceProperties();
  alignment = ezMath::Max<vk::DeviceSize>(16, alignment, properties.limits.nonCoherentAtomSize);
  size = ezMemoryUtils::AlignSize(size, alignment);

  ezStagingBufferVulkan buffer;

  EZ_ASSERT_DEBUG(m_device, "ezStagingBufferPoolVulkan::Initialize not called");
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
  m_pDevice->DeleteLater(buffer.m_buffer, buffer.m_alloc);

  // EZ_ASSERT_DEBUG(m_device, "ezStagingBufferPoolVulkan::Initialize not called");
  // ezMemoryAllocatorVulkan::DestroyBuffer(buffer.m_buffer, buffer.m_alloc);
}
