
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObjectBase<ezGALSwapChainCreationDescription>
{
public:

  /// Returns the render target view configuration.
  ezGALRenderTargetConfigHandle GetRenderTargetViewConfig() const;

  /// \brief Returns a handle to the back buffer texture.
  ///
  /// \attention Some RenderSystems do not support reading the swap chain's back buffer texture. Those will return an invalid handle.
  ezGALTextureHandle GetBackBufferTexture() const;

  /// \brief Returns a handle to the swap chain's depth buffer texture.
  ///
  /// \attention Some RenderSystems do not support reading the swap chain's back buffer depth buffer texture. Those will return an invalid handle.
  ezGALTextureHandle GetDepthStencilBufferTexture() const;


protected:

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChain();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice);

  void SetBackBufferObjects(ezGALRenderTargetConfigHandle hRenderTargetConfig, ezGALTextureHandle hBackBufferTexture, ezGALTextureHandle hDepthStencilBufferTexture);

  ezGALTextureHandle m_hBackBufferTexture;

  ezGALTextureHandle m_hDepthStencilBufferTexture;

  ezGALRenderTargetConfigHandle m_hRenderTargetConfig;
};

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>