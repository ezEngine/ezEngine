
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

class ezGALTextureUnorderedAccessViewWebGPU : public ezGALTextureUnorderedAccessView
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALTextureUnorderedAccessViewWebGPU(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description);
  ~ezGALTextureUnorderedAccessViewWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

class ezGALBufferUnorderedAccessViewWebGPU : public ezGALBufferUnorderedAccessView
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALBufferUnorderedAccessViewWebGPU(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description);
  ~ezGALBufferUnorderedAccessViewWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

#include <RendererWebGPU/Resources/Implementation/UnorderedAccessViewWebGPU_inl.h>
