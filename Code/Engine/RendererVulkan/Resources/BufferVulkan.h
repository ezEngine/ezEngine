
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALBufferVulkan : public ezGALBuffer
{
public:
  void DiscardBuffer() const;
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;

  EZ_ALWAYS_INLINE vk::IndexType GetIndexType() const;
  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;
  EZ_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  EZ_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;
  static vk::DeviceSize GetAlignment(const ezGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage);

protected:
  struct BufferVulkan
  {
    vk::Buffer m_buffer;
    ezVulkanAllocation m_alloc;
    mutable ezUInt64 m_currentFrame = 0;
  };

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferVulkan(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;
  void CreateBuffer() const;

  mutable BufferVulkan m_currentBuffer;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  mutable ezDeque<BufferVulkan> m_usedBuffers;
  mutable ezVulkanAllocationInfo m_allocInfo;

  // Data for memory barriers and access
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};
  vk::IndexType m_indexType = vk::IndexType::eUint16; // Only applicable for index buffers
  vk::BufferUsageFlags m_usage = {};
  vk::DeviceSize m_size = 0;

  ezGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device m_device;

  mutable ezString m_sDebugName;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
