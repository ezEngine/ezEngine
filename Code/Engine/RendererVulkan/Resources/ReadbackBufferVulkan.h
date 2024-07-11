#pragma once

#include <RendererFoundation/Resources/ReadbackBuffer.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

class ezGALReadbackBufferVulkan : public ezGALReadbackBuffer
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_buffer; }
  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const { return m_alloc; }
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const { return m_allocInfo; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALReadbackBufferVulkan(const ezGALBufferCreationDescription& Description);
  virtual ~ezGALReadbackBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_buffer = {};
  vk::DeviceSize m_size = 0;
  ezVulkanAllocation m_alloc = {};
  ezVulkanAllocationInfo m_allocInfo = {};

  ezGALDeviceVulkan* m_pDeviceVulkan = nullptr;
};
