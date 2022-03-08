#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

#include <d3d11.h>

ezGALBufferVulkan::ezGALBufferVulkan(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description)
  , m_buffer(nullptr)
  , m_indexType(vk::IndexType::eUint16)
{
}

ezGALBufferVulkan::~ezGALBufferVulkan() {}

ezResult ezGALBufferVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  m_pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::BufferCreateInfo bufferCreateInfo = {};

  switch (m_Description.m_BufferType)
  {
    case ezGALBufferType::ConstantBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
      break;
    case ezGALBufferType::IndexBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;

      m_indexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

      break;
    case ezGALBufferType::VertexBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
      break;
    case ezGALBufferType::Generic:
      //bufferCreateInfo.usage = 0; // TODO Is this correct for Vulkan?
      break;
    default:
      ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
      return EZ_FAILURE;
  }

  if (m_Description.m_bAllowShaderResourceView)
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;

  if (m_Description.m_bAllowUAV)
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;

  if (m_Description.m_bStreamOutputTarget)
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer; // TODO is this correct for vulkan?

  bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst; // TODO optimize this

  bufferCreateInfo.pQueueFamilyIndices = m_pDeviceVulkan->GetQueueFamilyIndices().GetPtr();
  bufferCreateInfo.queueFamilyIndexCount = m_pDeviceVulkan->GetQueueFamilyIndices().GetCount();
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_Description.m_uiTotalSize;
  //BufferDesc.CPUAccessFlags = 0;
  //BufferDesc.MiscFlags = 0;

  if (m_Description.m_bUseForIndirectArguments)
  {
    // TODO Vulkan?
  }

  if (m_Description.m_bAllowRawViews)
  {
    // TODO Vulkan?
  }

  if (m_Description.m_bUseAsStructuredBuffer)
  {
    // TODO Vulkan?
  }

  //BufferDesc.StructureByteStride = m_Description.m_uiStructSize;

  ezVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, m_buffer, m_alloc, &m_allocInfo));

  m_device = m_pDeviceVulkan->GetVulkanDevice(); // TODO remove this

  // TODO initial data upload
  return EZ_SUCCESS;
}

ezResult ezGALBufferVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_buffer)
  {
    ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
    pVulkanDevice->DeleteLater(m_buffer, m_alloc);
  }

  return EZ_SUCCESS;
}

void ezGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_buffer)
  {
    vk::DebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.objectType = m_buffer.objectType;
    nameInfo.objectHandle = (uint64_t)(VkBuffer)m_buffer;
    nameInfo.pObjectName = szName;

    m_device.setDebugUtilsObjectNameEXT(nameInfo);
    if (m_alloc)
      ezMemoryAllocatorVulkan::SetAllocationUserData(m_alloc, szName);
  }
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_BufferVulkan);
