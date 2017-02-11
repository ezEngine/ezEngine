
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObject<ezGALSwapChainCreationDescription>
{
public:

  /// \brief Returns a handle to the back buffer texture.
  ///
  /// \attention Some RenderSystems do not support reading the swap chain's back buffer texture. Those will return an invalid handle.
  inline ezGALTextureHandle GetBackBufferTexture() const;

protected:

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChain();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice);

  ezGALTextureHandle m_hBackBufferTexture;
};

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>