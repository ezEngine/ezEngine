#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/QueryPoolVulkan.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererVulkan/Device/DeviceVulkan.h>

ezQueryPoolVulkan::ezQueryPoolVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
  , m_TimestampPool(pDevice->GetAllocator())
  , m_OcclusionPool(pDevice->GetAllocator())
{
}

void ezQueryPoolVulkan::Initialize(ezUInt32 uiValidBits)
{
  m_TimestampPool.Initialize(m_pDevice->GetVulkanDevice(), 256, vk::QueryType::eTimestamp);
  m_OcclusionPool.Initialize(m_pDevice->GetVulkanDevice(), 32, vk::QueryType::eOcclusion);

  EZ_ASSERT_DEV(m_pDevice->GetPhysicalDeviceProperties().limits.timestampComputeAndGraphics, "Timestamps not supported by hardware.");
  m_fNanoSecondsPerTick = m_pDevice->GetPhysicalDeviceProperties().limits.timestampPeriod;

  m_uiValidBitsMask = uiValidBits == 64 ? ezMath::MaxValue<ezUInt64>() : ((1ull << uiValidBits) - 1);
}

void ezQueryPoolVulkan::DeInitialize()
{
  m_TimestampPool.DeInitialize();
  m_OcclusionPool.DeInitialize();
}

ezQueryPoolVulkan::Pool::Pool(ezAllocator* pAllocator)
  : m_pendingFrames(pAllocator)
  , m_resetPools(pAllocator)
  , m_freePools(pAllocator)
{
}

void ezQueryPoolVulkan::Pool::Initialize(vk::Device device, ezUInt32 uiPoolSize, vk::QueryType queryType)
{
  m_device = device;
  m_uiPoolSize = uiPoolSize;
  m_QueryType = queryType;
}

void ezQueryPoolVulkan::Pool::DeInitialize()
{
  for (FramePool& framePool : m_pendingFrames)
  {
    for (ezUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      m_freePools.PushBack(framePool.m_pools[i]);
    }
  }
  m_pendingFrames.Clear();
  for (QueryPool* pPool : m_freePools)
  {
    m_device.destroyQueryPool(pPool->m_pool);
    EZ_DEFAULT_DELETE(pPool);
  }
  m_freePools.Clear();
}

void ezQueryPoolVulkan::Calibrate()
{
  EZ_PROFILE_SCOPE("Calibrate");
  // #TODO_VULKAN Replace with VK_KHR_calibrated_timestamps
  // To correlate CPU to GPU time, we create an event, wait a bit for the GPU to get stuck on it and then signal it.
  // We then observe the time right after on the CPU and on the GPU via a timestamp query.
  vk::EventCreateInfo eventCreateInfo;
  vk::Event event;
  VK_ASSERT_DEV(m_TimestampPool.m_device.createEvent(&eventCreateInfo, nullptr, &event));

  m_TimestampPool.m_device.waitIdle();
  vk::CommandBuffer cb = m_pDevice->GetCurrentCommandBuffer();
  cb.waitEvents(1, &event, vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eTopOfPipe, 0, nullptr, 0, nullptr, 0, nullptr);
  auto hTimestamp = InsertTimestamp(cb, vk::PipelineStageFlagBits::eTopOfPipe);
  vk::Fence fence = m_pDevice->Submit();

  ezTime systemTS;
  ezThreadUtils::Sleep(ezTime::Milliseconds(100)); // Waiting for 100ms should be enough for the GPU to have gotten stuck on the event right?
  m_TimestampPool.m_device.setEvent(event);
  systemTS = ezTime::Now();

  VK_ASSERT_DEV(m_TimestampPool.m_device.waitForFences(1, &fence, true, ezMath::MaxValue<ezUInt64>()));

  ezTime gpuTS;
  EZ_VERIFY(GetTimestampResult(hTimestamp, gpuTS, true) == ezGALAsyncResult::Ready, "");
  m_gpuToCpuDelta = systemTS - gpuTS;

  m_TimestampPool.m_device.destroyEvent(event);
}

void ezQueryPoolVulkan::AfterBeginFrame(vk::CommandBuffer commandBuffer)
{
  ezUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();
  ezUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();
  {
    EZ_PROFILE_SCOPE("TimestampPool");
    m_TimestampPool.BeginFrame(commandBuffer, uiCurrentFrame, uiSafeFrame);
  }
  {
    EZ_PROFILE_SCOPE("OcclusionPool");
    m_OcclusionPool.BeginFrame(commandBuffer, uiCurrentFrame, uiSafeFrame);
  }
  if (m_gpuToCpuDelta.IsZero())
  {
    Calibrate();
  }
}

void ezQueryPoolVulkan::EnsureFreeQueryPoolSize(vk::CommandBuffer commandBuffer)
{
  m_OcclusionPool.EnsureFreeQueryPoolSize(commandBuffer, 1);
  m_TimestampPool.EnsureFreeQueryPoolSize(commandBuffer, 3);
}

void ezQueryPoolVulkan::Pool::BeginFrame(vk::CommandBuffer commandBuffer, ezUInt64 uiCurrentFrame, ezUInt64 uiSafeFrame)
{
  m_pCurrentFrame = &m_pendingFrames.ExpandAndGetRef();
  m_pCurrentFrame->m_uiFrameCounter = uiCurrentFrame;
  m_pCurrentFrame->m_uiNextIndex = 0;

  // Get results
  for (FramePool& framePool : m_pendingFrames)
  {
    // Skip checking if the corresponding frame fence has not been reached.
    if (framePool.m_uiFrameCounter > uiSafeFrame)
      break;

    bool bAllCompleted = true;
    for (ezUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      QueryPool* pPool = framePool.m_pools[i];

      if (!pPool->m_bReady)
      {
        bAllCompleted = false;
        ezUInt32 uiQueryCount = m_uiPoolSize;
        // The last pool may not be full, so we only query the number of queries that were actually used.
        const bool bLastPool = i + 1 == framePool.m_pools.GetCount();
        const ezUInt64 uiPoolEndIndex = m_uiPoolSize * (i + 1);
        const bool bPoolFull = framePool.m_uiNextIndex >= uiPoolEndIndex;
        if (bLastPool && !bPoolFull)
          uiQueryCount = framePool.m_uiNextIndex % m_uiPoolSize;

        EZ_ASSERT_DEV(uiQueryCount != 0, "Query pools are created on demand, so they should never have zero queries in them!");
        EZ_PROFILE_SCOPE("getQueryPoolResults");
        vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, 0, uiQueryCount, m_uiPoolSize * sizeof(ezUInt64), pPool->m_queryResults.GetData(), sizeof(ezUInt64), vk::QueryResultFlagBits::e64);
        if (res == vk::Result::eSuccess)
        {
          pPool->m_bReady = true;
        }
      }
    }

    if (bAllCompleted)
    {
      framePool.m_uiReadyFrames++;
    }
  }

  // Clear out old frames
  while (m_pendingFrames[0].m_uiReadyFrames > s_uiRetainFrames)
  {
    for (QueryPool* pPool : m_pendingFrames[0].m_pools)
    {
      pPool->m_bReady = false;
      pPool->m_queryResults.Clear();
      pPool->m_queryResults.SetCount(m_uiPoolSize, 0);
      m_freePools.PushBack(pPool);
      m_resetPools.PushBack(pPool->m_pool);
    }
    m_pendingFrames.PopFront();
  }

  m_uiFirstFrameIndex = m_pendingFrames[0].m_uiFrameCounter;

  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
  }
  m_resetPools.Clear();
}

void ezQueryPoolVulkan::Pool::EnsureFreeQueryPoolSize(vk::CommandBuffer commandBuffer, ezUInt32 uiFreePools)
{
  while (m_freePools.GetCount() < uiFreePools)
  {
    m_freePools.PushBack(CreatePool());
  }
  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
  }
  m_resetPools.Clear();
}

ezGALTimestampHandle ezQueryPoolVulkan::InsertTimestamp(vk::CommandBuffer commandBuffer, vk::PipelineStageFlagBits pipelineStage)
{
  ezGALTimestampHandle hTimestamp = m_TimestampPool.CreateQuery(commandBuffer);
  Query query = m_TimestampPool.GetQuery(hTimestamp);
  commandBuffer.writeTimestamp(pipelineStage, query.m_pool, query.uiQueryIndex);

  return hTimestamp;
}

ezGALPoolHandle ezQueryPoolVulkan::Pool::CreateQuery(vk::CommandBuffer commandBuffer)
{
  const ezUInt64 uiPoolIndex = m_pCurrentFrame->m_uiNextIndex / m_uiPoolSize;
  if (uiPoolIndex == m_pCurrentFrame->m_pools.GetCount())
  {
    m_pCurrentFrame->m_pools.PushBack(GetFreePool());
  }

  ezGALPoolHandle hPool = {m_pCurrentFrame->m_uiNextIndex, m_pCurrentFrame->m_uiFrameCounter};
  m_pCurrentFrame->m_uiNextIndex++;

  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
  }
  m_resetPools.Clear();

  return hPool;
}

ezQueryPoolVulkan::Query ezQueryPoolVulkan::Pool::GetQuery(ezGALPoolHandle hPool)
{
  EZ_ASSERT_DEBUG(hPool.m_Generation == m_pCurrentFrame->m_uiFrameCounter, "Timestamps must be created and used in the same frame!");
  const ezUInt32 uiPoolIndex = (ezUInt32)hPool.m_InstanceIndex / m_uiPoolSize;
  const ezUInt32 uiQueryIndex = (ezUInt32)hPool.m_InstanceIndex % m_uiPoolSize;
  return {m_pCurrentFrame->m_pools[uiPoolIndex]->m_pool, uiQueryIndex};
}

ezEnum<ezGALAsyncResult> ezQueryPoolVulkan::GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& out_result, bool bForce)
{
  ezUInt64 out_uiResult;
  ezEnum<ezGALAsyncResult> res = m_TimestampPool.GetResult(hTimestamp, out_uiResult, bForce);
  if (res == ezGALAsyncResult::Ready)
  {
    out_uiResult &= m_uiValidBitsMask;
    out_result = ezTime::Nanoseconds(m_fNanoSecondsPerTick * out_uiResult) + m_gpuToCpuDelta;
  }
  else
  {
    out_result = ezTime();
  }
  return res;
}

ezGALPoolHandle ezQueryPoolVulkan::BeginOcclusionQuery(vk::CommandBuffer commandBuffer, ezEnum<ezGALQueryType> type)
{
  ezGALPoolHandle hPool = m_OcclusionPool.CreateQuery(commandBuffer);
  Query query = m_OcclusionPool.GetQuery(hPool);
  commandBuffer.beginQuery(query.m_pool, query.uiQueryIndex, type == ezGALQueryType::NumSamplesPassed ? vk::QueryControlFlagBits::ePrecise : (vk::QueryControlFlagBits)0);

  return hPool;
}

void ezQueryPoolVulkan::EndOcclusionQuery(vk::CommandBuffer commandBuffer, ezGALPoolHandle hPool)
{
  Query query = m_OcclusionPool.GetQuery(hPool);
  commandBuffer.endQuery(query.m_pool, query.uiQueryIndex);
}

ezEnum<ezGALAsyncResult> ezQueryPoolVulkan::GetOcclusionQueryResult(ezGALPoolHandle hPool, ezUInt64& out_uiQueryResult, bool bForce)
{
  return m_OcclusionPool.GetResult(hPool, out_uiQueryResult, bForce);
}

ezEnum<ezGALAsyncResult> ezQueryPoolVulkan::Pool::GetResult(ezGALPoolHandle hPool, ezUInt64& out_uiResult, bool bForce)
{
  out_uiResult = 0;
  if (hPool.m_Generation >= m_uiFirstFrameIndex)
  {
    const ezUInt32 uiFrameIndex = static_cast<ezUInt32>(hPool.m_Generation - m_uiFirstFrameIndex);
    const ezUInt32 uiPoolIndex = (ezUInt32)hPool.m_InstanceIndex / m_uiPoolSize;
    const ezUInt32 uiQueryIndex = (ezUInt32)hPool.m_InstanceIndex % m_uiPoolSize;
    FramePool& framePools = m_pendingFrames[uiFrameIndex];
    QueryPool* pPool = framePools.m_pools[uiPoolIndex];
    if (pPool->m_bReady)
    {
      out_uiResult = pPool->m_queryResults[uiQueryIndex];
      return ezGALAsyncResult::Ready;
    }
    else if (bForce)
    {
      vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, uiQueryIndex, 1, sizeof(ezUInt64), &pPool->m_queryResults[uiQueryIndex], sizeof(ezUInt64), vk::QueryResultFlagBits::e64);
      if (res == vk::Result::eSuccess)
      {
        out_uiResult = pPool->m_queryResults[uiQueryIndex];
        return ezGALAsyncResult::Ready;
      }
    }
    return ezGALAsyncResult::Pending;
  }
  else
  {
    return ezGALAsyncResult::Expired;
  }
}

ezQueryPoolVulkan::QueryPool* ezQueryPoolVulkan::Pool::GetFreePool()
{
  if (!m_freePools.IsEmpty())
  {
    QueryPool* pPool = m_freePools.PeekBack();
    m_freePools.PopBack();
    return pPool;
  }

  return CreatePool();
}

ezQueryPoolVulkan::QueryPool* ezQueryPoolVulkan::Pool::CreatePool()
{
  vk::QueryPoolCreateInfo info;
  info.queryType = m_QueryType;
  info.queryCount = m_uiPoolSize;

  QueryPool* pPool = EZ_DEFAULT_NEW(QueryPool);
  pPool->m_queryResults.SetCount(m_uiPoolSize, 0);
  VK_ASSERT_DEV(m_device.createQueryPool(&info, nullptr, &pPool->m_pool));

  m_resetPools.PushBack(pPool->m_pool);
  return pPool;
}
