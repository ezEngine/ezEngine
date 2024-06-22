#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

/// \brief Pool for GPU queries.
class EZ_RENDERERVULKAN_DLL ezQueryPoolVulkan
{
public:
  /// \brief Initializes the pool.
  /// \param pDevice Parent Vulkan device.
  /// \param uiValidBits The number of valid bits in the query result. Each queue has different query characteristics and a separate pool is needed for each queue.
  void Initialize(ezGALDeviceVulkan* pDevice, ezUInt32 uiValidBits);
  void DeInitialize();

  /// \brief Needs to be called every frame so the pool can figure out which queries have finished and reuse old data.
  void BeginFrame(vk::CommandBuffer commandBuffer);

  /// \brief Inserts a timestamp into the given command buffer.
  /// \param commandBuffer Target command buffer to insert the timestamp into.
  /// \param hTimestamp Timestamp to insert. After insertion the only valid option is to call GetTimestampResult.
  /// \param pipelineStage The value of the timestamp will be the point in time in which all previously committed commands have finished this stage.
  ezGALTimestampHandle InsertTimestamp(vk::CommandBuffer commandBuffer, vk::PipelineStageFlagBits pipelineStage = vk::PipelineStageFlagBits::eBottomOfPipe);

  /// \brief Retrieves the timestamp value if it is available.
  /// \param hTimestamp The target timestamp to resolve.
  /// \param result The time of the timestamp. If this is empty on success the timestamp has expired.
  /// \param bForce Wait for the timestamp to become available.
  /// \return Returns false if the result is not available yet.
  ezEnum<ezGALAsyncResult> GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& out_result, bool bForce = false);

  ezGALPoolHandle BeginOcclusionQuery(vk::CommandBuffer commandBuffer, ezEnum<ezGALQueryType> type);
  void EndOcclusionQuery(vk::CommandBuffer commandBuffer, ezGALPoolHandle hPool);
  ezEnum<ezGALAsyncResult> GetOcclusionQueryResult(ezGALPoolHandle hPool, ezUInt64& out_uiQueryResult, bool bForce = false);

private:
  /// \brief GPU and CPU timestamps have no relation in Vulkan. To establish it we need to measure the same timestamp in both and compute the difference.
  void Calibrate();

  static constexpr ezUInt32 s_uiRetainFrames = 4;

  /// A singular query
  struct Query
  {
    vk::QueryPool m_pool;
    ezUInt32 uiQueryIndex;
  };

  /// A fixed number of queries are stored in this pool (see m_uiPoolSize below)
  /// These can be reset queried and reset as a whole. A good compromise must be found between API overhead and resource waste.
  struct QueryPool
  {
    vk::QueryPool m_pool;
    bool m_bReady = false;
    ezDynamicArray<uint64_t> m_queryResults;
  };

  /// Represents a frame of queries. Depending on the number of queries, multiple QueryPools will need to be used per frame.
  struct FramePool
  {
    ezUInt64 m_uiNextIndex = 0; // Next query index in this frame. Use % and / to find the pool / element index.
    ezUInt64 m_uiFrameCounter = 0; // ezGALDevice::GetCurrentFrame
    ezHybridArray<QueryPool*, 2> m_pools;
  };

  /// High level pool of Vulkan queries. Will create more sub-pools to manage demand, reusing them after s_uiRetainFrames of available results.
  /// If the user does not retrieve the values within s_uiRetainFrames time, the result expires.
  struct Pool
  {
    void Initialize(vk::Device device, ezUInt32 uiPoolSize, vk::QueryType queryType);
    void DeInitialize();

    QueryPool* GetFreePool();
    void BeginFrame(vk::CommandBuffer commandBuffer, ezUInt64 uiCurrentFrame, ezUInt64 uiSafeFrame);
    ezGALPoolHandle CreateQuery(vk::CommandBuffer commandBuffer);
    Query GetQuery(ezGALPoolHandle hPool);
    ezEnum<ezGALAsyncResult> GetResult(ezGALPoolHandle hPool, ezUInt64& out_uiResult, bool bForce);

    // Current active data.
    FramePool* m_pCurrentFrame = nullptr;
    ezUInt64 m_uiFirstFrameIndex = 0;
    ezDeque<FramePool> m_pendingFrames;

    // Pools.
    ezHybridArray<vk::QueryPool, 8> m_resetPools;
    ezHybridArray<QueryPool*, 8> m_freePools;

    vk::Device m_device;
    ezUInt32 m_uiPoolSize = 0;
    vk::QueryType m_QueryType;
  };

  ezGALDeviceVulkan* m_pDevice = nullptr;
  
  // Timestamp conversion and calibration data.
  double m_fNanoSecondsPerTick = 0;
  ezUInt64 m_uiValidBitsMask = 0;
  ezTime m_gpuToCpuDelta;

  // Pools
  Pool m_TimestampPool;
  Pool m_OcclusionPool;
};
