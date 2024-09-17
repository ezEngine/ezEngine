
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <webgpu/webgpu_cpp.h>

class ezGALTextureResourceViewWebGPU : public ezGALTextureResourceView
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;
  friend class ezGALCommandEncoderImplWebGPU;

  ezGALTextureResourceViewWebGPU(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description);
  ~ezGALTextureResourceViewWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  wgpu::TextureView m_TextureView;
};


class ezGALBufferResourceViewWebGPU : public ezGALBufferResourceView
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;
  friend class ezGALCommandEncoderImplWebGPU;

  ezGALBufferResourceViewWebGPU(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description);
  ~ezGALBufferResourceViewWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

#include <RendererWebGPU/Resources/Implementation/ResourceViewWebGPU_inl.h>
