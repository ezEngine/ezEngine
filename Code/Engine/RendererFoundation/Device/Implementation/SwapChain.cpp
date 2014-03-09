
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

ezGALSwapChain::ezGALSwapChain(const ezGALSwapChainCreationDescription& Description)
: ezGALObjectBase(Description)
{
}

ezGALSwapChain::~ezGALSwapChain()
{
}


ezResult ezGALSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  pDevice->DestroyRenderTargetView(m_hBackbBufferRenderTargetView);
  pDevice->DestroyTexture(m_hBackBufferTexture);

  return EZ_SUCCESS;
}

void ezGALSwapChain::SetBackBufferObjects(ezGALTextureHandle hBackBufferTexture, ezGALRenderTargetViewHandle hBackbBufferRenderTargetView)
{
  m_hBackBufferTexture = hBackBufferTexture;
  m_hBackbBufferRenderTargetView = hBackbBufferRenderTargetView;
}
