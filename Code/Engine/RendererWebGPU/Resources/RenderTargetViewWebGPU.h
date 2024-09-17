
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

class ezGALRenderTargetViewWebGPU : public ezGALRenderTargetView
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewWebGPU(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

#include <RendererWebGPU/Resources/Implementation/RenderTargetViewWebGPU_inl.h>
