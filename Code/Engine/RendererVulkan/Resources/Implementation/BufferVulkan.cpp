#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Device/InitContext.h>

ezGALBufferVulkan::ezGALBufferVulkan(const ezGALBufferCreationDescription& Description, bool bCPU)
  : ezGALBuffer(Description)
{
}

ezGALBufferVulkan::~ezGALBufferVulkan() {}

ezResult ezGALBufferVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  m_pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_device = m_pDeviceVulkan->GetVulkanDevice();
  m_stages = vk::PipelineStageFlagBits::eTransfer;

  const bool bSRV = m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource);
  const bool bUAV = m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess);
  for (ezGALBufferUsageFlags::Enum flag : m_Description.m_BufferFlags)
  {
    switch (flag)
    {
      case ezGALBufferUsageFlags::VertexBuffer:
        m_usage |= vk::BufferUsageFlagBits::eVertexBuffer;
        m_stages |= vk::PipelineStageFlagBits::eVertexInput;
        m_access |= vk::AccessFlagBits::eVertexAttributeRead;
        // EZ_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case ezGALBufferUsageFlags::IndexBuffer:
        m_usage |= vk::BufferUsageFlagBits::eIndexBuffer;
        m_stages |= vk::PipelineStageFlagBits::eVertexInput;
        m_access |= vk::AccessFlagBits::eIndexRead;
        m_indexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        // EZ_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case ezGALBufferUsageFlags::ConstantBuffer:
        m_usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        m_stages |= m_pDeviceVulkan->GetSupportedStages();
        m_access |= vk::AccessFlagBits::eUniformRead;
        break;
      case ezGALBufferUsageFlags::TexelBuffer:
        if (bSRV)
          m_usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        if (bUAV)
          m_usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        break;
      case ezGALBufferUsageFlags::StructuredBuffer:
      case ezGALBufferUsageFlags::ByteAddressBuffer:
        m_usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        break;
      case ezGALBufferUsageFlags::ShaderResource:
        m_stages |= m_pDeviceVulkan->GetSupportedStages();
        m_access |= vk::AccessFlagBits::eShaderRead;
        break;
      case ezGALBufferUsageFlags::UnorderedAccess:
        m_stages |= m_pDeviceVulkan->GetSupportedStages();
        m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
        break;
      case ezGALBufferUsageFlags::DrawIndirect:
        m_usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
        m_stages |= vk::PipelineStageFlagBits::eDrawIndirect;
        m_access |= vk::AccessFlagBits::eIndirectCommandRead;
        break;
      default:
        ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
        return EZ_FAILURE;
    }
  }

  m_usage |= vk::BufferUsageFlagBits::eTransferSrc;
  m_access |= vk::AccessFlagBits::eTransferRead;
  m_usage |= vk::BufferUsageFlagBits::eTransferDst;
  m_access |= vk::AccessFlagBits::eTransferWrite;

  EZ_ASSERT_DEBUG(pInitialData.GetCount() <= m_Description.m_uiTotalSize, "Initial data is bigger than target buffer.");
  vk::DeviceSize alignment = GetAlignment(m_pDeviceVulkan, m_usage);
  m_size = ezMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  m_resourceBufferInfo.offset = 0;
  m_resourceBufferInfo.range = m_size;

  if (m_Description.m_ResourceAccess.m_MemoryUsage == ezGALMemoryUsage::Dynamic)
  {
    EZ_ASSERT_DEV(m_Description.m_BufferFlags == ezGALBufferUsageFlags::ConstantBuffer, "Only basic constant buffers support dynamic memory");
    return EZ_SUCCESS;
  }

  CreateBuffer();

  if (!pInitialData.IsEmpty())
  {
    m_pDeviceVulkan->GetInitContext().InitBuffer(this, pInitialData);
  }
  return EZ_SUCCESS;
}

ezResult ezGALBufferVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_buffer, m_alloc);
    m_allocInfo = {};
  }

  m_resourceBufferInfo = vk::DescriptorBufferInfo();

  m_stages = {};
  m_access = {};
  m_indexType = vk::IndexType::eUint16;
  m_usage = {};
  m_size = 0;

  m_pDeviceVulkan = nullptr;
  m_device = nullptr;

  return EZ_SUCCESS;
}

const vk::DescriptorBufferInfo& ezGALBufferVulkan::GetBufferInfo() const
{
  return m_resourceBufferInfo;
}

void ezGALBufferVulkan::CreateBuffer()
{
  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = m_usage;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_size;

  ezVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;

  switch (m_Description.m_ResourceAccess.m_MemoryUsage)
  {
    case ezGALMemoryUsage::GPU:
      break;
    case ezGALMemoryUsage::Staging:
      allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped;
      break;
    case ezGALMemoryUsage::Readback:
      allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessRandom | ezVulkanAllocationCreateFlags::Mapped;
      break;
    case ezGALMemoryUsage::Dynamic:
      // #TODO_VULKAN don't actually create anything here, dynamic can only be used for transient buffers via new API function
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_buffer, m_alloc, &m_allocInfo));
  m_resourceBufferInfo.buffer = m_buffer;
}

void ezGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_sDebugName = szName;
  m_pDeviceVulkan->SetDebugName(szName, m_buffer, m_alloc);
}

vk::DeviceSize ezGALBufferVulkan::GetAlignment(const ezGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage)
{
  const vk::PhysicalDeviceProperties& properties = pDevice->GetPhysicalDeviceProperties();

  vk::DeviceSize alignment = ezMath::Max<vk::DeviceSize>(4, properties.limits.nonCoherentAtomSize);

  if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
    alignment = ezMath::Max(alignment, properties.limits.minUniformBufferOffsetAlignment);

  if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
    alignment = ezMath::Max(alignment, properties.limits.minStorageBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer))
    alignment = ezMath::Max(alignment, properties.limits.minTexelBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndirectBuffer))
    alignment = ezMath::Max(alignment, VkDeviceSize(16)); // If no cache line aligned perf will suffer.

  if (usage & (vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst))
    alignment = ezMath::Max(alignment, properties.limits.optimalBufferCopyOffsetAlignment);

  return alignment;
}
