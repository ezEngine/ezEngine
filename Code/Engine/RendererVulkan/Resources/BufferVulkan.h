
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


protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferVulkan(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezGALDeviceVulkan* m_pDeviceVulkan = nullptr;

  vk::Buffer m_buffer;
  ezVulkanAllocation m_alloc;
  ezVulkanAllocationInfo m_allocInfo;

  vk::Device m_device;

  vk::IndexType m_indexType; // Only applicable for index buffers
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
