#pragma once

#include <RendererDX11/Resources/TextureDX11.h>

struct IDXGIKeyedMutex;

class ezGALSharedTextureDX11 : public ezGALTextureDX11, public ezGALSharedTexture
{
  using SUPER = ezGALTextureDX11;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALSharedTextureDX11(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle);
  ~ezGALSharedTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual ezGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(ezUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(ezUInt64 uiValue) const override;

protected:
  ezEnum<ezGALSharedTextureType> m_SharedType = ezGALSharedTextureType::None;
  ezGALPlatformSharedHandle m_hSharedHandle;
  IDXGIKeyedMutex* m_pKeyedMutex = nullptr;
};
