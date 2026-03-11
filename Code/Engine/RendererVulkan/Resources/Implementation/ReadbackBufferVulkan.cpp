#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackBufferVulkan.h>

ezGALReadbackBufferVulkan::ezGALReadbackBufferVulkan(const ezGALBufferCreationDescription& Description)
  : ezGALReadbackBuffer(Description)
{
}

ezGALReadbackBufferVulkan::~ezGALReadbackBufferVulkan() = default;

ezResult ezGALReadbackBufferVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::DeviceSize alignment = ezGALBufferVulkan::GetAlignment(m_pDeviceVulkan, vk::BufferUsageFlagBits::eTransferDst);
  m_size = ezMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_size;

  ezVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessRandom | ezVulkanAllocationCreateFlags::Mapped;

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_buffer, m_alloc, &m_allocInfo));

  return EZ_SUCCESS;
}

ezResult ezGALReadbackBufferVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_buffer, m_alloc);
    m_allocInfo = {};
  }
  m_size = 0;
  m_pDeviceVulkan = nullptr;
  return EZ_SUCCESS;
}

void ezGALReadbackBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDeviceVulkan->SetDebugName(szName, m_buffer, m_alloc);
}
