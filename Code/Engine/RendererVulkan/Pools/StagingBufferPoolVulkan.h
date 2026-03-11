#pragma once

#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>


class ezGALDeviceVulkan;



/// \brief Allocates temporary staging buffers from a large pool. Allocations will automatically be freed at the end of the frame.
/// New (larger) pools will be created if the existing ones run out of space. Pools will be deleted after a certain time of no usage.
class EZ_RENDERERVULKAN_DLL ezStagingBufferPoolVulkan
{
public:
  /// \brief Initializes the pool.
  /// \param pDevice GAL device.
  /// \param uiStartingPoolSize Size of the first pool. If depleted, a new one with twice the size of the previous one is created.
  void Initialize(ezGALDeviceVulkan* pDevice, ezUInt64 uiStartingPoolSize);
  /// \brief Needs to be called before destroying this instance. Ensure that the GPU is idle before calling.
  void DeInitialize();

  /// \brief Needs to be called after begin frame to free memory.
  void AfterBeginFrame();
  /// \brief Needs to be called before submitting work to the GPU to flush GPU caches.
  void BeforeCommandBufferSubmit();

  /// \brief Allocates a temp buffer of the given size.
  /// \param size The size of the temp buffer.
  /// \return Allocated temp buffer.
  ezStagingBufferVulkan AllocateBuffer(ezUInt64 size);

private:
  static constexpr ezUInt32 s_uiNumberOfFramesToKeepUnusedPoolsAlive = 600;

  struct StagingBufferPool
  {
    StagingBufferPool(ezUInt32 uiAlignment, ezUInt32 uiTotalSize);
    ~StagingBufferPool();
    ezResult Allocate(ezUInt32 uiSize, ezUInt64 uiCurrentFrame, ezUInt32& out_uiStartOffset, ezByteArrayPtr& out_Allocation);
    void Free(ezUInt64 uiUpToFrame);
    void Submit(ezGALDeviceVulkan* pDevice, ezUInt64 uiFrame);

    ezRingBufferTracker m_Tracker;
    ezArrayPtr<ezUInt8> m_Data;
    vk::Buffer m_Buffer;
    ezVulkanAllocation m_Alloc;
    ezVulkanAllocationInfo m_AllocInfo;
    ezUInt32 m_uiFramesWithoutAllocations = 0;
  };

private:
  StagingBufferPool* GetFreePool(ezUInt64 uiSize);

private:
  ezUInt64 m_uiAlignment = 0;
  ezUInt64 m_uiStartingPoolSize = 10 * 1024u * 1024u;
  ezGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;

  ezHybridArray<StagingBufferPool*, 8> m_Pools;
  ezUInt64 m_uiHighWatermark = 0;
};
