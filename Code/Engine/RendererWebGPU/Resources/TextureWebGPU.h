#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <webgpu/webgpu_cpp.h>

class ezGALDeviceWebGPU;

class ezGALTextureWebGPU : public ezGALTexture
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezGALSharedTextureWebGPU;
  friend class ezMemoryUtils;
  friend class ezGALTextureResourceViewWebGPU;

  ezGALTextureWebGPU(const ezGALTextureCreationDescription& Description);
  ~ezGALTextureWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  wgpu::Texture m_Texture;
};

#include <RendererWebGPU/Resources/Implementation/TextureWebGPU_inl.h>
