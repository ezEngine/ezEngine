
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class ezGALSwapChainDX11 : public ezGALSwapChain
{
public:

  EZ_FORCE_INLINE IDXGISwapChain* GetDXSwapChain() const;

protected:

  friend class ezGALDeviceDX11;

  ezGALSwapChainDX11(const ezGALSwapChainCreationDescription& Description, ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackBufferRenderTargetView, IDXGISwapChain* pDXSwapChain);

  virtual ~ezGALSwapChainDX11();

  IDXGISwapChain* m_pDXSwapChain;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
