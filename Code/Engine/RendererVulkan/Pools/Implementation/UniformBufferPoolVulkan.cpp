#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Device/InitContext.h>

ezUniformBufferPoolVulkan::ezUniformBufferPoolVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
  , m_device(pDevice->GetVulkanDevice())
  , m_Buffer(pDevice->GetAllocator())
  , m_PendingPools(pDevice->GetAllocator())
  , m_FreePools(pDevice->GetAllocator())
{
}

void ezUniformBufferPoolVulkan::Initialize()
{
  m_uiAlignment = ezGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
}

void ezUniformBufferPoolVulkan::DeInitialize()
{
  if (m_pCurrentPool)
  {
    m_FreePools.PushBack(m_pCurrentPool);
    m_pCurrentPool = nullptr;
  }

  for (UniformBufferPool* pPool : m_PendingPools)
  {
    m_FreePools.PushBack(pPool);
  }
  m_PendingPools.Clear();

  for (UniformBufferPool* pPool : m_FreePools)
  {
    ezFoundation::GetAlignedAllocator()->Deallocate(pPool->m_Data.GetPtr());
    pPool->m_Data.Clear();
    m_pDevice->DeleteLater(pPool->m_Buffer, pPool->m_Alloc);
    if (pPool->m_StagingBuffer)
    {
      m_pDevice->DeleteLater(pPool->m_StagingBuffer, pPool->m_StagingAlloc);
    }
    EZ_DELETE(m_pDevice->GetAllocator(), pPool);
  }
  m_FreePools.Clear();
}

void ezUniformBufferPoolVulkan::Reset(vk::CommandBuffer commandBuffer)
{
  m_Buffer.Clear();
  ezUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();
  while (!m_PendingPools.IsEmpty() && m_PendingPools.PeekFront()->m_uiFrameCounter <= uiSafeFrame)
  {
    UniformBufferPool* pPool = m_PendingPools.PeekFront();
    m_PendingPools.PopFront();
    pPool->m_uiCurrentOffset = 0;
    pPool->m_uiFrameCounter = 0;

    m_FreePools.PushBack(pPool);
  }
}

void ezUniformBufferPoolVulkan::Submit(vk::CommandBuffer commandBuffer)
{
  if (m_pCurrentPool && m_pCurrentPool->m_uiCurrentOffset != 0)
  {
    m_pCurrentPool->Submit(m_pDevice);
    m_PendingPools.PushBack(m_pCurrentPool);
    m_pCurrentPool = nullptr;
  }
}

ezUniformBufferPoolVulkan::BufferUpdateResult ezUniformBufferPoolVulkan::UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> data)
{
  BufferUpdateResult res = BufferUpdateResult::OffsetChanged;
  const ezUInt32 uiSize = ezMemoryUtils::AlignSize(data.GetCount(), (ezUInt32)m_uiAlignment);
  if (!m_pCurrentPool)
  {
    m_pCurrentPool = GetFreePool();
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  if (m_pCurrentPool->m_uiCurrentOffset + uiSize > m_pCurrentPool->m_uiSize)
  {
    m_pCurrentPool->Submit(m_pDevice);
    m_PendingPools.PushBack(m_pCurrentPool);
    m_pCurrentPool = GetFreePool();
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  EZ_ASSERT_DEBUG(m_pCurrentPool->m_uiCurrentOffset + uiSize <= m_pCurrentPool->m_uiSize, "Uniform buffer too big or implementation error");
  ezMemoryUtils::RawByteCopy(m_pCurrentPool->m_Data.GetPtr() + m_pCurrentPool->m_uiCurrentOffset, data.GetPtr(), data.GetCount());

  vk::DescriptorBufferInfo info;
  info.buffer = m_pCurrentPool->m_Buffer;
  info.range = m_pCurrentPool->m_uiSize;
  info.offset = m_pCurrentPool->m_uiCurrentOffset;
  m_Buffer.Insert(pBuffer, info);

  m_pCurrentPool->m_uiCurrentOffset += uiSize;
  return res;
}

vk::DescriptorBufferInfo ezUniformBufferPoolVulkan::GetBuffer(const ezGALBufferVulkan* pBuffer) const
{
  vk::DescriptorBufferInfo info;
  EZ_ASSERT_DEBUG(m_Buffer.TryGetValue(pBuffer, info), "Dynamic buffer not found. Dynamic buffers must be updated every frame before use.");
  return info;
}

ezUniformBufferPoolVulkan::UniformBufferPool* ezUniformBufferPoolVulkan::GetFreePool()
{
  if (!m_FreePools.IsEmpty())
  {
    ezUniformBufferPoolVulkan::UniformBufferPool* pPool = m_FreePools.PeekBack();
    m_FreePools.PopBack();
    return pPool;
  }

  // Create new pPool
  const ezUInt32 uiMaxBufferSize = m_pDevice->GetPhysicalDeviceProperties().limits.maxUniformBufferRange;
  vk::DeviceSize uiSize = ezMath::Min(1024u * 1024u, uiMaxBufferSize);

  uiSize = ezMemoryUtils::AlignSize(uiSize, m_uiAlignment);
  while (uiSize >= uiMaxBufferSize)
  {
    uiSize -= m_uiAlignment;
  }

  UniformBufferPool* pPool(EZ_NEW(m_pDevice->GetAllocator(), UniformBufferPool));
  pPool->m_uiSize = uiSize;
  pPool->m_uiFrameCounter = m_pDevice->GetCurrentFrame();
  pPool->m_Data = ezMakeArrayPtr(static_cast<ezUInt8*>(ezFoundation::GetAlignedAllocator()->Allocate(uiSize, m_uiAlignment)), uiSize);

  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiSize;

    ezVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped | ezVulkanAllocationCreateFlags::AllowTransferInstead;
    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, pPool->m_Buffer, pPool->m_Alloc, &pPool->m_AllocInfo));
  }

  vk::MemoryPropertyFlags memFlags = ezMemoryAllocatorVulkan::GetAllocationFlags(pPool->m_Alloc);
  if (!(memFlags & vk::MemoryPropertyFlagBits::eHostVisible))
  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiSize;

    ezVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped;
    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, pPool->m_StagingBuffer, pPool->m_StagingAlloc, &pPool->m_StagingAllocInfo));
  }

  return pPool;
}

void ezUniformBufferPoolVulkan::UniformBufferPool::Submit(ezGALDeviceVulkan* pDevice)
{
  if (m_StagingBuffer)
  {
    ezMemoryUtils::RawByteCopy(m_StagingAllocInfo.m_pMappedData, m_Data.GetPtr(), m_uiCurrentOffset);
    vk::Result result = ezMemoryAllocatorVulkan::FlushAllocation(m_StagingAlloc, 0, VK_WHOLE_SIZE);
    VK_ASSERT_DEBUG(result);

    pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, m_uiCurrentOffset);
  }
  else
  {
    ezMemoryUtils::RawByteCopy(m_AllocInfo.m_pMappedData, m_Data.GetPtr(), m_uiCurrentOffset);
    VK_ASSERT_DEBUG(ezMemoryAllocatorVulkan::FlushAllocation(m_StagingAlloc, 0, VK_WHOLE_SIZE));

    pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, m_uiCurrentOffset);
  }
}
