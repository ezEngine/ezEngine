
#pragma once

#include <RendererFoundation/Basics.h>

class EZ_RENDERERFOUNDATION_DLL ezGALSwapChain : public ezGALObjectBase<ezGALSwapChainCreationDescription>
{
public:

  ezGALTextureHandle GetTexture() const
  {
    return m_hBackBufferTexture;
  }

  ezGALRenderTargetViewHandle GetRenderTargetView() const
  {
    return m_hBackbBufferRenderTargetView;
  }

protected:

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChain();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice);

  void SetBackBufferObjects(ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackbBufferRenderTargetView);

  ezGALTextureHandle m_hBackBufferTexture;

  ezGALRenderTargetViewHandle m_hBackbBufferRenderTargetView;
};
