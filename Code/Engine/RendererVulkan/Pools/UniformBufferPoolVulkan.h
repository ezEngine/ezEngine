#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

class EZ_RENDERERVULKAN_DLL ezUniformBufferPoolVulkan
{
public:
  enum class BufferUpdateResult
  {
    DynamicBufferChanged,
    OffsetChanged
  };

  ezUniformBufferPoolVulkan(ezGALDeviceVulkan* pDevice);

  void Initialize();
  void DeInitialize();

  void Reset(vk::CommandBuffer commandBuffer);
  void Submit(vk::CommandBuffer commandBuffer);

  BufferUpdateResult UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> data);
  vk::DescriptorBufferInfo GetBuffer(const ezGALBufferVulkan* pBuffer) const;

private:
  struct UniformBufferPool
  {
    void Submit(ezGALDeviceVulkan* pDevice);

    ezUInt32 m_uiCurrentOffset = 0;
    ezUInt32 m_uiSize = 0;
    ezUInt64 m_uiFrameCounter = 0;
    ezArrayPtr<ezUInt8> m_Data;
    vk::Buffer m_Buffer;
    vk::Buffer m_StagingBuffer;
    ezVulkanAllocation m_Alloc;
    ezVulkanAllocation m_StagingAlloc;
    ezVulkanAllocationInfo m_AllocInfo;
    ezVulkanAllocationInfo m_StagingAllocInfo;
  };

private:
  UniformBufferPool* GetFreePool();

private:
  ezGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
  vk::DeviceSize m_uiAlignment = 0;

  ezHashTable<const ezGALBufferVulkan*, vk::DescriptorBufferInfo> m_Buffer;

  UniformBufferPool* m_pCurrentPool = nullptr;
  ezDeque<UniformBufferPool*> m_PendingPools;
  ezHybridArray<UniformBufferPool*, 8> m_FreePools;
};