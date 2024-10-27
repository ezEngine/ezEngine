#pragma once

#include <RendererFoundation/Resources/ReadbackTexture.h>

#include <RendererVulkan/Resources/TextureVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALDeviceVulkan;

class ezGALReadbackTextureVulkan : public ezGALReadbackTexture
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_buffer; }
  EZ_ALWAYS_INLINE ezVulkanAllocation GetBufferAllocation() const { return m_bufferAlloc; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALReadbackTextureVulkan(const ezGALTextureCreationDescription& Description);
  ~ezGALReadbackTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_buffer = {};
  ezVulkanAllocation m_bufferAlloc;
  ezVulkanAllocationInfo m_bufferAllocInfo;

  ezGALDeviceVulkan* m_pDevice = nullptr;
};
