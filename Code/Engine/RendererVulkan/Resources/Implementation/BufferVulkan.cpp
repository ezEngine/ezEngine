#include <RendererVulkanPCH.h>

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
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

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

  bufferCreateInfo.pQueueFamilyIndices = pVulkanDevice->GetQueueFamilyIndices().GetPtr();
  bufferCreateInfo.queueFamilyIndexCount = pVulkanDevice->GetQueueFamilyIndices().GetCount();
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

  m_buffer = pVulkanDevice->GetVulkanDevice().createBuffer(bufferCreateInfo);

  if (!m_buffer)
  {
    return EZ_FAILURE;
  }

  vk::MemoryPropertyFlags bufferMemoryProperties = {};

  if (m_Description.m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    bufferMemoryProperties |= vk::MemoryPropertyFlagBits::eDeviceLocal;

    // TODO do we need to use this for vulkan?
    // If constant buffer: Patch size to be aligned to 64 bytes for easier usability
   // BufferDesc.ByteWidth = ezMemoryUtils::AlignSize(BufferDesc.ByteWidth, 64u);
  }
  else
  {
    // TODO is this flag relevant to Vulkan?
    /*if (m_Description.m_ResourceAccess.IsImmutable())
    {
      // TODO vulkan
      //BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else*/
    {
      // for performance reasons we'll require shader accessed buffers to reside in device
      // memory.
      if (m_Description.m_bAllowUAV ||
        m_Description.m_bAllowShaderResourceView ||
        m_Description.m_bStreamOutputTarget ||
        m_Description.m_ResourceAccess.m_bReadBack) 
        bufferMemoryProperties |= vk::MemoryPropertyFlagBits::eDeviceLocal;
      }
      else
      {
        bufferMemoryProperties |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
      }
    }
  }
  
  vk::MemoryRequirements bufferMemoryRequirements = pVulkanDevice->GetVulkanDevice().getBufferMemoryRequirements(m_buffer);

  vk::MemoryAllocateInfo memoryAllocateInfo = {};
  memoryAllocateInfo.allocationSize = bufferMemoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = pVulkanDevice->GetMemoryIndex(bufferMemoryProperties, bufferMemoryRequirements);

  m_memory = pVulkanDevice->GetVulkanDevice().allocateMemory(memoryAllocateInfo);
  m_memoryOffset = 0;// TODO suballocations

  if (!m_memory)
  {
    pVulkanDevice->GetVulkanDevice().destroyBuffer(m_buffer);
    m_buffer = nullptr;

    return EZ_FAILURE;
  }

  pVulkanDevice->GetVulkanDevice().bindBufferMemory(m_buffer, m_memory, m_memoryOffset);
  m_device = pVulkanDevice->GetVulkanDevice(); // TODO remove this

  // TODO initial data upload
  return EZ_SUCCESS;

}

ezResult ezGALBufferVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_buffer)
  {
    ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

    pVulkanDevice->GetVulkanDevice().destroyBuffer(m_buffer);
    pVulkanDevice->GetVulkanDevice().freeMemory(m_memory);

    m_buffer = nullptr;
    m_memory = nullptr;
    m_memoryOffset = 0;
  }


  return EZ_SUCCESS;
}

void ezGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_buffer)
  {
    vk::DebugMarkerObjectNameInfoEXT nameInfo = {};
    nameInfo.object = (uint64_t)(VkBuffer)m_buffer;
    nameInfo.objectType = vk::DebugReportObjectTypeEXT::eBuffer;
    nameInfo.pObjectName = szName;

    m_device.debugMarkerSetObjectNameEXT(nameInfo);
  }
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_BufferVulkan);
