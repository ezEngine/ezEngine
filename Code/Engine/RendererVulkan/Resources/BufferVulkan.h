
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALBufferVulkan : public ezGALBuffer
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;

  EZ_ALWAYS_INLINE vk::IndexType GetIndexType() const;
  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;
  EZ_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  EZ_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferVulkan(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;
  vk::DeviceSize GetAlignment(const ezGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage) const;

  vk::Buffer m_buffer;
  ezVulkanAllocation m_alloc;
  ezVulkanAllocationInfo m_allocInfo;

  // Data for memory barriers and access
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};
  vk::IndexType m_indexType; // Only applicable for index buffers

  ezGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device m_device;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
