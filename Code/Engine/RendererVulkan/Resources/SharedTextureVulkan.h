#pragma once

#include <RendererVulkan/Resources/TextureVulkan.h>

class ezGALSharedTextureVulkan : public ezGALTextureVulkan, public ezGALSharedTexture
{
  using SUPER = ezGALTextureVulkan;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALSharedTextureVulkan(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle);
  ~ezGALSharedTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual ezGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(ezUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(ezUInt64 uiValue) const override;

protected:
  ezEnum<ezGALSharedTextureType> m_SharedType = ezGALSharedTextureType::None;
  ezGALPlatformSharedHandle m_SharedHandle;
  vk::Semaphore m_SharedSemaphore;
};
