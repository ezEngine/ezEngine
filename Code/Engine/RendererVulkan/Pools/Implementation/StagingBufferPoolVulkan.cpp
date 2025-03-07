#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void ezStagingBufferPoolVulkan::Initialize(ezGALDeviceVulkan* pDevice, ezUInt64 uiStartingPoolSize)
{
  m_pDevice = pDevice;
  m_device = pDevice->GetVulkanDevice();

  const vk::PhysicalDeviceProperties& properties = m_pDevice->GetPhysicalDeviceProperties();
  m_uiAlignment = ezMath::Max<ezUInt64>(16ull, m_uiAlignment, (ezUInt64)properties.limits.nonCoherentAtomSize);
  m_uiAlignment = ezMath::Max(m_uiAlignment, (ezUInt64)properties.limits.optimalBufferCopyOffsetAlignment);
  EZ_ASSERT_DEBUG(ezMath::IsPowerOf2(m_uiAlignment), "Non-power of two alignment not supported");
  m_uiStartingPoolSize = ezMemoryUtils::AlignSize(uiStartingPoolSize, m_uiAlignment);
}

void ezStagingBufferPoolVulkan::DeInitialize()
{
  // We assume the device makes sure no buffers are still in use before calling this.
  m_device = nullptr;
  for (StagingBufferPool* pPool : m_Pools)
  {
    EZ_DELETE(m_pDevice->GetAllocator(), pPool);
  }
  m_Pools.Clear();

  ezLog::Info("StagingBufferPoolVulkan#{}: high water mark: {}", ezArgP(this), ezArgHumanReadable((ezInt64)m_uiHighWatermark));
}

void ezStagingBufferPoolVulkan::AfterBeginFrame()
{
  ezUInt64 m_uiTotalAllocatedSize = 0;
  const ezUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();
  if (uiSafeFrame >= 1)
  {
    for (StagingBufferPool* pPool : m_Pools)
    {
      m_uiTotalAllocatedSize += pPool->m_Tracker.GetUsedMemory();
      // The -1 is necessary because we use some staging buffers for two frames, see comments in ezGALDeviceVulkan::UpdateBufferForNextFramePlatform and UpdateTextureForNextFramePlatform.
      pPool->Free(uiSafeFrame - 1);
    }
  }
  m_uiHighWatermark = ezMath::Max<ezUInt64>(m_uiTotalAllocatedSize, m_uiHighWatermark);

  // Keep one pool around at all times.
  while (m_Pools.GetCount() > 1 && m_Pools.PeekBack()->m_uiFramesWithoutAllocations > s_uiNumberOfFramesToKeepUnusedPoolsAlive)
  {
    // It is safe to delete the pool here as there can't be any usage on the GPU of the buffer if the pool is unused.
    StagingBufferPool* pPool = m_Pools.PeekBack();
    m_Pools.PopBack();
    EZ_DELETE(m_pDevice->GetAllocator(), pPool);
  }
}

void ezStagingBufferPoolVulkan::BeforeCommandBufferSubmit()
{
  for (StagingBufferPool* pPool : m_Pools)
  {
    pPool->Submit(m_pDevice, m_pDevice->GetCurrentFrame());
  }
}

ezStagingBufferVulkan ezStagingBufferPoolVulkan::AllocateBuffer(ezUInt64 size)
{
  EZ_ASSERT_DEBUG(m_device, "ezStagingBufferPoolVulkan::Initialize not called");
  ezStagingBufferVulkan buffer;

  ezUInt32 uiOffset = 0;
  ezByteArrayPtr allocation;
  for (StagingBufferPool* pPool : m_Pools)
  {
    if (pPool->Allocate((ezUInt32)size, m_pDevice->GetCurrentFrame(), uiOffset, allocation).Succeeded())
    {
      buffer.m_alloc = pPool->m_Alloc;
      buffer.m_buffer = pPool->m_Buffer;
      break;
    }
  }

  if (allocation.IsEmpty())
  {
    StagingBufferPool* pPool = GetFreePool(size);
    pPool->Allocate((ezUInt32)size, m_pDevice->GetCurrentFrame(), uiOffset, allocation).AssertSuccess("Newly created pool should be able to allocate requested size");
    buffer.m_alloc = pPool->m_Alloc;
    buffer.m_buffer = pPool->m_Buffer;
  }

  buffer.m_Data = allocation;
  buffer.m_uiOffset = uiOffset;
  return buffer;
}

ezStagingBufferPoolVulkan::StagingBufferPool* ezStagingBufferPoolVulkan::GetFreePool(ezUInt64 uiSize)
{
  ezUInt64 uiNewPoolSize = m_Pools.IsEmpty() ? m_uiStartingPoolSize : m_Pools.PeekBack()->m_Tracker.GetTotalMemory() * 2;

  m_uiStartingPoolSize = ezMemoryUtils::AlignSize(ezMath::Max(uiNewPoolSize, uiSize), m_uiAlignment);

  StagingBufferPool* pPool = EZ_NEW(m_pDevice->GetAllocator(), StagingBufferPool, (ezUInt32)m_uiAlignment, (ezUInt32)m_uiStartingPoolSize);
  m_Pools.PushBack(pPool);
  return pPool;
}


ezStagingBufferPoolVulkan::StagingBufferPool::StagingBufferPool(ezUInt32 uiAlignment, ezUInt32 uiTotalSize)
  : m_Tracker(uiAlignment, uiTotalSize)
{
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size = uiTotalSize;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

  ezVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
  allocInfo.m_flags = ezVulkanAllocationCreateFlags::HostAccessSequentialWrite | ezVulkanAllocationCreateFlags::Mapped;

  VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, m_Buffer, m_Alloc, &m_AllocInfo));
  m_Data = ezMakeArrayPtr(reinterpret_cast<ezUInt8*>(m_AllocInfo.m_pMappedData), uiTotalSize);
}

ezStagingBufferPoolVulkan::StagingBufferPool::~StagingBufferPool()
{
  ezMemoryAllocatorVulkan::DestroyBuffer(m_Buffer, m_Alloc);
}

ezResult ezStagingBufferPoolVulkan::StagingBufferPool::Allocate(ezUInt32 uiSize, ezUInt64 uiCurrentFrame, ezUInt32& out_uiStartOffset, ezByteArrayPtr& out_Allocation)
{
  if (m_Tracker.Allocate(uiSize, uiCurrentFrame, out_uiStartOffset).Failed())
    return EZ_FAILURE;

  out_Allocation = m_Data.GetSubArray(out_uiStartOffset, uiSize);
  return EZ_SUCCESS;
}

void ezStagingBufferPoolVulkan::StagingBufferPool::Free(ezUInt64 uiUpToFrame)
{
  m_uiFramesWithoutAllocations = m_Tracker.GetUsedMemory() == 0 ? ++m_uiFramesWithoutAllocations : 0;
  m_Tracker.Free(uiUpToFrame);
}

void ezStagingBufferPoolVulkan::StagingBufferPool::Submit(ezGALDeviceVulkan* pDevice, ezUInt64 uiFrame)
{
  ezHybridArray<ezRingBufferTracker::FrameData, 4> frameData;
  if (m_Tracker.SubmitFrame(uiFrame, frameData).Failed())
    return;

  for (ezRingBufferTracker::FrameData& frame : frameData)
  {
    VK_ASSERT_DEBUG(ezMemoryAllocatorVulkan::FlushAllocation(m_Alloc, frame.m_uiStartOffset, frame.m_uiSize));
  }
}
