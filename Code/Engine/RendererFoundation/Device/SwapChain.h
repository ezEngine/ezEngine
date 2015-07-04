
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObjectBase<ezGALSwapChainCreationDescription>
{
public:

  /// \brief Returns a handle to the back buffer texture.
  ///
  /// \attention Some RenderSystems do not support reading the swap chain's back buffer texture. Those will return an invalid handle.
  inline ezGALTextureHandle GetBackBufferTexture() const;

  /// \brief Returns a handle to the render target view which can be bound to render to this swap chain.
  inline ezGALRenderTargetViewHandle GetBackBufferRenderTargetView() const;

  /// \brief Returns a handle to the swap chain's depth buffer texture.
  ///
  /// \attention Some RenderSystems do not support reading the swap chain's back buffer depth buffer texture. Those will return an invalid handle.
  inline ezGALTextureHandle GetDepthStencilBufferTexture() const;

  /// \brief Returns a handle to the swap chain's depth stencil view.
  ///
  /// \attention Only a swap chain which was created with m_bCreateDepthStencilBuffer or one which represents a native integrated depth buffer will return a valid handle;
  inline ezGALRenderTargetViewHandle GetDepthStencilTargetView() const;


protected:

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChain();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice);

  ezGALTextureHandle m_hBackBufferTexture;
  ezGALRenderTargetViewHandle m_hBackBufferRTV;

  ezGALTextureHandle m_hDepthStencilBufferTexture;
  ezGALRenderTargetViewHandle m_hBackBufferDSV;
};

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>