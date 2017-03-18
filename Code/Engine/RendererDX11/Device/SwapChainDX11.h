
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
  friend class ezMemoryUtils;

  ezGALSwapChainDX11(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;


  IDXGISwapChain* m_pDXSwapChain;

  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  ezGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
