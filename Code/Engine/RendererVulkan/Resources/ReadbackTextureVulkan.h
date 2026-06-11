#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/ReadbackTexture.h>

class ezGALBufferVulkan;
class ezGALDeviceVulkan;

class ezGALReadbackTextureVulkan : public ezGALReadbackTexture
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_Buffer; }
  EZ_ALWAYS_INLINE ezVulkanAllocation GetBufferAllocation() const { return m_pBufferAlloc; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALReadbackTextureVulkan(const ezGALTextureCreationDescription& Description);
  ~ezGALReadbackTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_Buffer = {};
  ezVulkanAllocation m_pBufferAlloc;
  ezVulkanAllocationInfo m_BufferAllocInfo;

  ezGALDeviceVulkan* m_pDevice = nullptr;
};
