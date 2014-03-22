
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObjectBase<ezGALSwapChainCreationDescription>
{
public:

  ezGALTextureHandle GetTexture() const
  {
    return m_hBackBufferTexture;
  }

  ezGALRenderTargetViewHandle GetRenderTargetView() const
  {
    return m_hBackBufferRenderTargetView;
  }

protected:

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChain();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice);

  void SetBackBufferObjects(ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackBufferRenderTargetView);

  ezGALTextureHandle m_hBackBufferTexture;

  ezGALRenderTargetViewHandle m_hBackBufferRenderTargetView;
};
