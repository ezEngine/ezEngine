#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ReadbackTextureVulkan.h>

ezGALReadbackTextureVulkan::ezGALReadbackTextureVulkan(const ezGALTextureCreationDescription& Description)
  : ezGALReadbackTexture(Description)
{
}

ezGALReadbackTextureVulkan::~ezGALReadbackTextureVulkan() = default;

ezResult ezGALReadbackTextureVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_pDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

  ezHybridArray<ezGALTextureVulkan::SubResourceOffset, 8> subResourceSizes;
  bufferCreateInfo.size = ezGALTextureVulkan::ComputeSubResourceOffsets(m_pDevice, m_Description, subResourceSizes);

  ezVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessRandom /*| ezVulkanAllocationCreateFlags::Mapped*/;

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_buffer, m_bufferAlloc, &m_bufferAllocInfo));
  return EZ_SUCCESS;
}

ezResult ezGALReadbackTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_buffer)
  {
    ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
    pVulkanDevice->DeleteLater(m_buffer, m_bufferAlloc);
    m_bufferAllocInfo = {};
    m_buffer = nullptr;
  }

  return EZ_SUCCESS;
}

void ezGALReadbackTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_buffer, m_bufferAlloc);
}
