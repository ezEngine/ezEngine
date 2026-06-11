#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/ReadbackBuffer.h>

class ezGALDeviceVulkan;

class ezGALReadbackBufferVulkan : public ezGALReadbackBuffer
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_Buffer; }
  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const { return m_pAlloc; }
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const { return m_AllocInfo; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALReadbackBufferVulkan(const ezGALBufferCreationDescription& Description);
  virtual ~ezGALReadbackBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_Buffer = {};
  vk::DeviceSize m_Size = 0;
  ezVulkanAllocation m_pAlloc = {};
  ezVulkanAllocationInfo m_AllocInfo = {};

  ezGALDeviceVulkan* m_pDeviceVulkan = nullptr;
};
