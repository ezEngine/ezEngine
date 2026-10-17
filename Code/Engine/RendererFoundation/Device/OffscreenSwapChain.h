#pragma once

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

/// Swap chain backed by a render-target texture without a presentation surface.
class EZ_RENDERERFOUNDATION_DLL ezGALOffscreenSwapChain : public ezGALSwapChain
{
  friend class ezGALDevice;

public:
  using Functor = ezDelegate<ezGALSwapChainHandle(const ezGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);
  static ezGALSwapChainHandle Create(const ezGALWindowSwapChainCreationDescription& desc);

protected:
  ezGALOffscreenSwapChain(const ezGALWindowSwapChainCreationDescription& desc);

  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) override;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) override;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) override;
  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezResult CreateBackBuffer(ezGALDevice* pDevice);
  void DestroyBackBuffer(ezGALDevice* pDevice);

  static Functor s_Factory;

  ezGALWindowSwapChainCreationDescription m_Desc;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALOffscreenSwapChain);
