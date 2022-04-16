#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

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

  m_stages = vk::PipelineStageFlagBits::eTransfer;
  vk::BufferCreateInfo bufferCreateInfo = {};

  switch (m_Description.m_BufferType)
  {
    case ezGALBufferType::ConstantBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
      m_stages |= m_pDeviceVulkan->GetSupportedStages();
      m_access |= vk::AccessFlagBits::eUniformRead;
      break;
    case ezGALBufferType::IndexBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
      m_stages |= vk::PipelineStageFlagBits::eVertexInput;
      m_access |= vk::AccessFlagBits::eIndexRead;
      m_indexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

      break;
    case ezGALBufferType::VertexBuffer:
      bufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
      m_stages |= vk::PipelineStageFlagBits::eVertexInput;
      m_access |= vk::AccessFlagBits::eVertexAttributeRead;
      break;
    case ezGALBufferType::Generic:
      m_stages |= m_pDeviceVulkan->GetSupportedStages();
      //bufferCreateInfo.usage = 0; //#TODO_VULKAN Is this correct for Vulkan?
      break;
    default:
      ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
      return EZ_FAILURE;
  }

  if (m_Description.m_bAllowShaderResourceView)
  {
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
  }

  if (m_Description.m_bAllowUAV)
  {
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
  }

  if (m_Description.m_bStreamOutputTarget)
  {
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
    //#TODO_VULKAN will need to create a counter buffer.
    m_stages |= vk::PipelineStageFlagBits::eTransformFeedbackEXT;
    m_access |= vk::AccessFlagBits::eTransformFeedbackWriteEXT;
  }

  if (m_Description.m_bUseForIndirectArguments)
  {
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    m_stages |= vk::PipelineStageFlagBits::eDrawIndirect;
    m_access |= vk::AccessFlagBits::eIndirectCommandRead;
  }

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    m_access |= vk::AccessFlagBits::eTransferRead;
  }

  bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eTransferDst;
  m_access |= vk::AccessFlagBits::eTransferWrite;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

  EZ_ASSERT_DEBUG(pInitialData.GetCount() <= m_Description.m_uiTotalSize, "Initial data is bigger than target buffer.");
  vk::DeviceSize alignment = GetAlignment(m_pDeviceVulkan, bufferCreateInfo.usage);
  vk::DeviceSize newSize = ezMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);
  bufferCreateInfo.size = newSize;


  if (m_Description.m_bAllowRawViews)
  {
    // TODO Vulkan?
  }

  if (m_Description.m_bUseAsStructuredBuffer)
  {
    // TODO Vulkan?
  }

  ezVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocInfo.m_flags = {};
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, m_buffer, m_alloc, &m_allocInfo));

  m_device = m_pDeviceVulkan->GetVulkanDevice(); // TODO remove this

  if (!pInitialData.IsEmpty())
    m_pDeviceVulkan->UploadBuffer(this, pInitialData);
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

vk::DeviceSize ezGALBufferVulkan::GetAlignment(const ezGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage) const
{
  const vk::PhysicalDeviceProperties& properties = pDevice->GetPhysicalDeviceProperties();

  vk::DeviceSize alignment = 4;

  if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
    alignment = ezMath::Max(alignment, properties.limits.minUniformBufferOffsetAlignment);

  if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
    alignment = ezMath::Max(alignment, properties.limits.minStorageBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer))
    alignment = ezMath::Max(alignment, properties.limits.minTexelBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndirectBuffer))
    alignment = ezMath::Max(alignment, VkDeviceSize(16));

  if (usage & (vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst))
    alignment = ezMath::Max(alignment, properties.limits.optimalBufferCopyOffsetAlignment);

  //#TODO_VULKAN Do we need to take nonCoherentAtomSize into account?
  return alignment;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_BufferVulkan);
