#pragma once

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Utils/RingBufferTracker.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;
class ezGALBufferVulkan;

/// \brief `ezGALBufferVulkan` created with `ezGALBufferUsageFlags::Transient` will allocate scratch memory from this pool which will last until the end of the frame.
class EZ_RENDERERVULKAN_DLL ezUniformBufferPoolVulkan
{
public:
  enum class BufferUpdateResult
  {
    OffsetChanged,        ///< The offset of the current buffer was moved forward.
    DynamicBufferChanged, ///< The current buffer was depleted and a new buffer was started.
  };

  ezUniformBufferPoolVulkan(ezGALDeviceVulkan* pDevice);

  void Initialize();
  void DeInitialize();

  void EndFrame();
  void BeforeCommandBufferSubmit();

  /// \brief Needs to be called each from for a buffer before it can be used.
  /// Can be called multiple times per frame. GetBuffer will always return a reference to the last UpdateBuffer content.
  /// \param pBuffer The buffer to allocate scratch memory for.
  /// \param data The data of the buffer. Must be the size of the entire buffer.
  /// \return Returns whether a new pool needed to be created.
  BufferUpdateResult UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> data);

  /// Access the descriptor info for the given buffer.
  /// \param pBuffer The buffer for which previously scratch memory was allocated for.
  /// \return Pointer to the descriptor info.
  const vk::DescriptorBufferInfo* GetBuffer(const ezGALBufferVulkan* pBuffer) const;

private:
  struct UniformBufferPool
  {
    UniformBufferPool(ezUInt32 uiAlignment, ezUInt32 uiTotalSize);
    ~UniformBufferPool();
    ezResult Allocate(ezUInt32 uiSize, ezUInt64 uiCurrentFrame, ezUInt32& out_uiStartOffset, ezByteArrayPtr& out_Allocation);
    void Free(ezUInt64 uiUpToFrame);
    void Submit(ezGALDeviceVulkan* pDevice, ezUInt64 uiFrame);
    ezUInt32 GetFreeMemory() const { return m_Tracker.GetFreeMemory(); }

    ezRingBufferTracker m_Tracker;
    ezArrayPtr<ezUInt8> m_Data;
    vk::Buffer m_Buffer;
    vk::Buffer m_StagingBuffer;
    ezVulkanAllocation m_Alloc;
    ezVulkanAllocation m_StagingAlloc;
    ezVulkanAllocationInfo m_AllocInfo;
    ezVulkanAllocationInfo m_StagingAllocInfo;
  };

private:
  UniformBufferPool* GetFreePool(ezUInt32 uiSize);
  ezUInt32 GetBufferSize(ezUInt32 uiSize);

private:
  ezGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
  ezUInt32 m_uiAlignment = 0;
  ezUInt32 m_uiBufferSize = 0;

  ezMap<const ezGALBufferVulkan*, vk::DescriptorBufferInfo> m_Buffer;

  UniformBufferPool* m_pCurrentPool = nullptr;
  ezDeque<UniformBufferPool*> m_PendingPools;
  ezHybridArray<UniformBufferPool*, 8> m_FreePools;
};
