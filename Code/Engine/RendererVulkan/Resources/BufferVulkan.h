
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALBufferVulkan : public ezGALBuffer
{
public:

  EZ_ALWAYS_INLINE vk::Buffer GetVulkanBuffer() const;

  EZ_ALWAYS_INLINE vk::IndexType GetIndexFormat() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferVulkan(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  vk::Buffer m_vulkanBuffer;

  vk::IndexType m_IndexFormat; // Only applicable for index buffers
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
