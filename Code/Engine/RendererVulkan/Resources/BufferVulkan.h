
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALBufferVulkan : public ezGALBuffer
{
public:
  EZ_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;

  EZ_ALWAYS_INLINE vk::IndexType GetIndexType() const;

  EZ_ALWAYS_INLINE vk::DeviceMemory GetMemory() const;
  EZ_ALWAYS_INLINE vk::DeviceSize GetMemoryOffset() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferVulkan(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  vk::Buffer m_buffer;
  vk::DeviceMemory m_memory;
  vk::DeviceSize m_memoryOffset;
  vk::Device m_device;

  vk::IndexType m_indexType; // Only applicable for index buffers
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
