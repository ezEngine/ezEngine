
#pragma once

#include <RendererFoundation/Basics.h>

class ezGALSwapChain : public ezGALObjectBase<ezGALSwapChainCreationDescription>
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

  ezGALSwapChain(const ezGALSwapChainCreationDescription& Description, ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackbBufferRenderTargetView)
    : ezGALObjectBase(Description), m_hBackBufferTexture(hBackBufferTexture), m_hBackbBufferRenderTargetView(hBackbBufferRenderTargetView)
  {
  }

  virtual ~ezGALSwapChain()
  {
  }

  ezGALTextureHandle m_hBackBufferTexture;

  ezGALRenderTargetViewHandle m_hBackbBufferRenderTargetView;
};
