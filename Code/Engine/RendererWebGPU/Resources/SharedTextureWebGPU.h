#pragma once

#include <RendererWebGPU/Resources/TextureWebGPU.h>

class ezGALSharedTextureWebGPU : public ezGALTextureWebGPU, public ezGALSharedTexture
{
  using SUPER = ezGALTextureWebGPU;

protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALSharedTextureWebGPU(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle);
  ~ezGALSharedTextureWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual ezGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(ezUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(ezUInt64 uiValue) const override;
};
