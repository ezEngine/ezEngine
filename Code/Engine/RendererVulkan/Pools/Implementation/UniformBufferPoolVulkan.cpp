#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

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
  m_uiAlignment = (ezUInt32)ezGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
  m_uiBufferSize = GetBufferSize(2u * 1024u * 1024u);
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
    EZ_DELETE(m_pDevice->GetAllocator(), pPool);
  }
  m_FreePools.Clear();
}

void ezUniformBufferPoolVulkan::EndFrame()
{
  m_Buffer.Clear();
  ezUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();

  for (UniformBufferPool* pPool : m_PendingPools)
  {
    m_FreePools.PushBack(pPool);
  }
  m_PendingPools.Clear();

  for (UniformBufferPool* pPool : m_FreePools)
  {
    pPool->Free(uiSafeFrame);
  }

  m_FreePools.Sort([](const UniformBufferPool* pLhs, const UniformBufferPool* pRhs) -> bool
    { return pLhs->GetFreeMemory() < pRhs->GetFreeMemory(); });

  // Don't switch the current pool as that would cause descriptor re-creation later on.
  if (m_pCurrentPool)
  {
    m_pCurrentPool->Free(uiSafeFrame);
  }
}

void ezUniformBufferPoolVulkan::BeforeCommandBufferSubmit()
{
  if (m_pCurrentPool)
  {
    m_pCurrentPool->Submit(m_pDevice, m_pDevice->GetCurrentFrame());
  }
}

ezUniformBufferPoolVulkan::BufferUpdateResult ezUniformBufferPoolVulkan::UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> data)
{
  const ezUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();
  BufferUpdateResult res = BufferUpdateResult::OffsetChanged;
  const ezUInt32 uiSize = ezMemoryUtils::AlignSize(data.GetCount(), (ezUInt32)m_uiAlignment);
  if (!m_pCurrentPool)
  {
    m_pCurrentPool = GetFreePool(uiSize);
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  ezUInt32 uiOffset = 0;
  ezByteArrayPtr allocation;
  if (m_pCurrentPool->Allocate(uiSize, uiCurrentFrame, uiOffset, allocation).Failed())
  {
    m_pCurrentPool->Submit(m_pDevice, uiCurrentFrame);
    m_PendingPools.PushBack(m_pCurrentPool);
    m_pCurrentPool = GetFreePool(uiSize);
    m_pCurrentPool->Allocate(uiSize, uiCurrentFrame, uiOffset, allocation).AssertSuccess("implementation error");
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  EZ_ASSERT_DEBUG(allocation.GetCount() >= data.GetCount(), "implementation error");
  ezMemoryUtils::RawByteCopy(allocation.GetPtr(), data.GetPtr(), data.GetCount());

  vk::DescriptorBufferInfo info;
  info.buffer = m_pCurrentPool->m_Buffer;
  info.range = uiSize;
  info.offset = uiOffset;
  m_Buffer.Insert(pBuffer, info);

  return res;
}

const vk::DescriptorBufferInfo* ezUniformBufferPoolVulkan::GetBuffer(const ezGALBufferVulkan* pBuffer) const
{
  vk::DescriptorBufferInfo* info = nullptr;
  bool bFound = m_Buffer.TryGetValue(pBuffer, info);
  EZ_ASSERT_DEBUG(bFound, "Dynamic buffer not found. Dynamic buffers must be updated every frame before use.");
  return info;
}

ezUniformBufferPoolVulkan::UniformBufferPool* ezUniformBufferPoolVulkan::GetFreePool(ezUInt32 uiSize)
{
  if (!m_FreePools.IsEmpty())
  {
    // The back has the pool with the highest available memory. If this fails there is no point in checking other pools.
    ezUniformBufferPoolVulkan::UniformBufferPool* pPool = m_FreePools.PeekBack();
    if (pPool->GetFreeMemory() >= uiSize)
    {
      m_FreePools.PopBack();
      return pPool;
    }
  }

  // Create new pool. This time larger in the hopes that it won't run out.
  m_uiBufferSize = GetBufferSize(m_uiBufferSize * 2);
  UniformBufferPool* pPool(EZ_NEW(m_pDevice->GetAllocator(), UniformBufferPool, m_uiAlignment, m_uiBufferSize));
  return pPool;
}


ezUInt32 ezUniformBufferPoolVulkan::GetBufferSize(ezUInt32 uiSize)
{
  const ezUInt32 uiMaxBufferSize = m_pDevice->GetPhysicalDeviceProperties().limits.maxUniformBufferRange;
  ezUInt32 uiBufferSize = ezMath::Min(uiSize, uiMaxBufferSize);
  uiBufferSize = ezMemoryUtils::AlignSize(uiBufferSize, m_uiAlignment);
  while (uiBufferSize >= uiMaxBufferSize)
  {
    uiBufferSize -= m_uiAlignment;
  }
  return uiBufferSize;
}

ezUniformBufferPoolVulkan::UniformBufferPool::UniformBufferPool(ezUInt32 uiAlignment, ezUInt32 uiBufferSize)
  : m_Tracker(uiAlignment, uiBufferSize)
{
  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiBufferSize;

    ezVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped | ezVulkanAllocationCreateFlags::AllowTransferInstead;
    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_Alloc, &m_AllocInfo));
  }

  vk::MemoryPropertyFlags memFlags = ezMemoryAllocatorVulkan::GetAllocationFlags(m_Alloc);
  if (memFlags & vk::MemoryPropertyFlagBits::eHostVisible)
  {
    m_Data = ezMakeArrayPtr(reinterpret_cast<ezUInt8*>(m_AllocInfo.m_pMappedData), uiBufferSize);
  }
  else
  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiBufferSize;

    ezVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = ezVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped;
    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_StagingBuffer, m_StagingAlloc, &m_StagingAllocInfo));
    m_Data = ezMakeArrayPtr(reinterpret_cast<ezUInt8*>(m_StagingAllocInfo.m_pMappedData), uiBufferSize);
  }
}


ezUniformBufferPoolVulkan::UniformBufferPool::~UniformBufferPool()
{
  m_Data.Clear();
  ezMemoryAllocatorVulkan::DestroyBuffer(m_Buffer, m_Alloc);
  if (m_StagingBuffer)
  {
    ezMemoryAllocatorVulkan::DestroyBuffer(m_StagingBuffer, m_StagingAlloc);
  }
}

ezResult ezUniformBufferPoolVulkan::UniformBufferPool::Allocate(ezUInt32 uiSize, ezUInt64 uiCurrentFrame, ezUInt32& out_uiStartOffset, ezByteArrayPtr& out_Allocation)
{
  if (m_Tracker.Allocate(uiSize, uiCurrentFrame, out_uiStartOffset).Failed())
    return EZ_FAILURE;

  out_Allocation = m_Data.GetSubArray(out_uiStartOffset, uiSize);
  return EZ_SUCCESS;
}

void ezUniformBufferPoolVulkan::UniformBufferPool::Free(ezUInt64 uiUpToFrame)
{
  m_Tracker.Free(uiUpToFrame);
}

void ezUniformBufferPoolVulkan::UniformBufferPool::Submit(ezGALDeviceVulkan* pDevice, ezUInt64 uiFrame)
{
  ezHybridArray<ezRingBufferTracker::FrameData, 4> frameData;
  if (m_Tracker.SubmitFrame(uiFrame, frameData).Failed())
    return;

  for (ezRingBufferTracker::FrameData& frame : frameData)
  {
    if (m_StagingBuffer)
    {
      VK_ASSERT_DEBUG(ezMemoryAllocatorVulkan::FlushAllocation(m_StagingAlloc, frame.m_uiStartOffset, frame.m_uiSize));
      EZ_ASSERT_DEBUG(frame.m_uiStartOffset + frame.m_uiSize <= m_Tracker.GetTotalMemory(), "Buffer overrun");
      pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, frame.m_uiStartOffset, frame.m_uiSize);
    }
    else
    {
      VK_ASSERT_DEBUG(ezMemoryAllocatorVulkan::FlushAllocation(m_Alloc, frame.m_uiStartOffset, frame.m_uiSize));
      EZ_ASSERT_DEBUG(frame.m_uiStartOffset + frame.m_uiSize <= m_Tracker.GetTotalMemory(), "Buffer overrun");
      pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, frame.m_uiStartOffset, frame.m_uiSize);
    }
  }
}
