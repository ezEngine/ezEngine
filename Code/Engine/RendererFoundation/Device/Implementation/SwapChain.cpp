
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetConfig.h>

ezGALSwapChain::ezGALSwapChain(const ezGALSwapChainCreationDescription& Description)
: ezGALObjectBase(Description)
{
}

ezGALSwapChain::~ezGALSwapChain()
{
}


ezResult ezGALSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  const ezGALRenderTargetConfig* pRenderTargetConfig = pDevice->GetRenderTargetConfig(m_hRenderTargetConfig);
  if (pRenderTargetConfig != nullptr)
  {
    for (ezUInt32 i = 0; i < pRenderTargetConfig->GetDescription().m_uiColorTargetCount; ++i)
      pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hColorTargets[i]);
    pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hDepthStencilTarget);

    pDevice->DestroyRenderTargetConfig(m_hRenderTargetConfig);
  }
  
  pDevice->DestroyTexture(m_hBackBufferTexture);
  pDevice->DestroyTexture(m_hDepthStencilBufferTexture);

  return EZ_SUCCESS;
}

void ezGALSwapChain::SetBackBufferObjects(ezGALRenderTargetConfigHandle hRenderTargetConfig, ezGALTextureHandle hBackBufferTexture, ezGALTextureHandle hDepthStencilBufferTexture)
{
  m_hRenderTargetConfig = hRenderTargetConfig;
  m_hBackBufferTexture = hBackBufferTexture;
  m_hBackBufferTexture = hDepthStencilBufferTexture;
}
