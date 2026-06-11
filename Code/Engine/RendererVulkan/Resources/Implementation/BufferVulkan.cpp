#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALBufferVulkan::ezGALBufferVulkan(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description)
{
}

ezGALBufferVulkan::~ezGALBufferVulkan() = default;

ezResult ezGALBufferVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  m_pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_Device = m_pDeviceVulkan->GetVulkanDevice();

  // Derive stages and access from the GAL default state, then mask to supported stages.
  const ezBitflags<ezGALResourceState> defaultState = m_Description.GetDefaultState();
  ezConversionUtilsVulkan::ConvertResourceState(defaultState, m_Stages, m_Access);
  m_Stages &= m_pDeviceVulkan->GetSupportedStages();

  const bool bSRV = m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource);
  const bool bUAV = m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess);
  for (ezGALBufferUsageFlags::Enum flag : m_Description.m_BufferFlags)
  {
    switch (flag)
    {
      case ezGALBufferUsageFlags::VertexBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eVertexBuffer;
        // EZ_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case ezGALBufferUsageFlags::IndexBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eIndexBuffer;
        m_IndexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        // EZ_ASSERT_DEBUG(!bSRV && !bUAV, "Not implemented");
        break;
      case ezGALBufferUsageFlags::ConstantBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        break;
      case ezGALBufferUsageFlags::TexelBuffer:
        if (bSRV)
          m_Usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        if (bUAV)
          m_Usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        break;
      case ezGALBufferUsageFlags::StructuredBuffer:
      case ezGALBufferUsageFlags::ByteAddressBuffer:
        m_Usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        break;
      case ezGALBufferUsageFlags::ShaderResource:
        break;
      case ezGALBufferUsageFlags::UnorderedAccess:
        break;
      case ezGALBufferUsageFlags::DrawIndirect:
        m_Usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
        break;
      case ezGALBufferUsageFlags::Transient:
        break;
      default:
        ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
        return EZ_FAILURE;
    }
  }

  m_Usage |= vk::BufferUsageFlagBits::eTransferSrc;
  m_Usage |= vk::BufferUsageFlagBits::eTransferDst;

  EZ_ASSERT_DEBUG(pInitialData.GetCount() <= m_Description.m_uiTotalSize, "Initial data is bigger than target buffer.");
  vk::DeviceSize alignment = GetAlignment(m_pDeviceVulkan, m_Usage);
  m_Size = ezMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  m_ResourceBufferInfo.offset = 0;
  m_ResourceBufferInfo.range = m_Size;

  // No buffer needed if we use transient memory for constant buffers.
  if (m_Description.m_BufferFlags.AreAllSet(ezGALBufferUsageFlags::Transient | ezGALBufferUsageFlags::ConstantBuffer))
    return EZ_SUCCESS;

  CreateBuffer();

  if (!pInitialData.IsEmpty())
  {
    m_pDeviceVulkan->GetInitContext().InitBuffer(this, pInitialData);
  }
  return EZ_SUCCESS;
}

ezResult ezGALBufferVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_Buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_Buffer, m_pAlloc);
    m_AllocInfo = {};
  }

  for (auto it : m_TexelBufferViews)
  {
    m_pDeviceVulkan->DeleteLater(it.Value());
  }
  m_TexelBufferViews.Clear();

  m_ResourceBufferInfo = vk::DescriptorBufferInfo();

  m_Stages = {};
  m_Access = {};
  m_IndexType = vk::IndexType::eUint16;
  m_Usage = {};
  m_Size = 0;

  m_pDeviceVulkan = nullptr;
  m_Device = nullptr;

  return EZ_SUCCESS;
}

const vk::DescriptorBufferInfo& ezGALBufferVulkan::GetBufferInfo() const
{
  return m_ResourceBufferInfo;
}

void ezGALBufferVulkan::CreateBuffer()
{
  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = m_Usage;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_Size;

  ezVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_pAlloc, &m_AllocInfo));
  m_ResourceBufferInfo.buffer = m_Buffer;
}

void ezGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_sDebugName = szName;
  m_pDeviceVulkan->SetDebugName(szName, m_Buffer, m_pAlloc);
}

vk::BufferView ezGALBufferVulkan::GetTexelBufferView(ezGALBufferRange bufferRange, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat) const
{
  vk::BufferView bufferView;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_TexelBufferViews.TryGetValue(view, bufferView))
  {
    const ezGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == ezGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

    vk::BufferViewCreateInfo viewCreateInfo;
    viewCreateInfo.buffer = m_Buffer;
    viewCreateInfo.offset = bufferRange.m_uiByteOffset;
    viewCreateInfo.range = bufferRange.m_uiByteCount;
    if (viewCreateInfo.range == EZ_GAL_WHOLE_SIZE)
      viewCreateInfo.range = VK_WHOLE_SIZE;
    viewCreateInfo.format = m_pDeviceVulkan->GetFormatLookupTable().GetFormatInfo(viewFormat).m_format;
    VK_ASSERT_DEV(m_Device.createBufferView(&viewCreateInfo, nullptr, &bufferView));
    m_TexelBufferViews.Insert(view, bufferView);
  }

  return bufferView;
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
