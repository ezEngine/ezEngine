#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class ezGALTextureVulkan : public ezGALTexture
{
public:

  EZ_ALWAYS_INLINE vk::Image GetImage() const;

  EZ_ALWAYS_INLINE ezGALBufferVulkan* GetStagingBuffer() const;

  EZ_ALWAYS_INLINE vk::DeviceMemory GetMemory() const;

  EZ_ALWAYS_INLINE vk::DeviceSize GetMemoryOffset() const;
  EZ_ALWAYS_INLINE vk::DeviceSize GetMemorySize() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult ReplaceExisitingNativeObject(void* pExisitingNativeObject) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult CreateStagingTexture(ezGALDeviceVulkan* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

  void* m_pExisitingNativeObject = nullptr;

};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
