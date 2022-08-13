
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObject<ezGALSwapChainCreationDescription>
{
public:
  const ezGALRenderTargets& GetRenderTargets() const { return m_RenderTargets; }
  ezGALTextureHandle GetBackBufferTexture() const { return m_RenderTargets.m_hRTs[0]; }

  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) = 0;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) = 0;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) = 0;

  virtual ~ezGALSwapChain();

protected:
  friend class ezGALDevice;

  ezGALSwapChain(const ezRTTI* pSwapChainType);

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALRenderTargets m_RenderTargets;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALSwapChain);


class EZ_RENDERERFOUNDATION_DLL ezGALWindowSwapChain : public ezGALSwapChain
{
public:
  using Functor = ezDelegate<ezGALSwapChainHandle(const ezGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static ezGALSwapChainHandle Create(const ezGALWindowSwapChainCreationDescription& desc);

public:
  const ezGALWindowSwapChainCreationDescription& GetWindowDescription() const { return m_WindowDesc; }

protected:
  ezGALWindowSwapChain(const ezGALWindowSwapChainCreationDescription& Description);

protected:
  static Functor s_Factory;

protected:
  ezGALWindowSwapChainCreationDescription m_WindowDesc;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALWindowSwapChain);

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>
