#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;

class ezGALTextureVulkan : public ezGALTexture
{
public:
  EZ_ALWAYS_INLINE vk::Image GetImage() const;

  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;

  EZ_ALWAYS_INLINE const ezGALBufferVulkan* GetStagingBuffer() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult ReplaceExisitingNativeObject(void* pExisitingNativeObject) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult CreateStagingBuffer(ezGALDeviceVulkan* pDevice);

  vk::Image m_image;
  ezVulkanAllocation m_alloc;
  ezVulkanAllocationInfo m_allocInfo;

  // TODO can we hold a pointer to the buffer indefinitely?
  ezGALBufferHandle m_stagingBufferHandle;
  const ezGALBufferVulkan* m_pStagingBuffer = nullptr;

  vk::Device m_device;

  void* m_pExisitingNativeObject;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
