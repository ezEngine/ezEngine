
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetConfig.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

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
    {
      /// \todo The texture was a memory leak, but I have no idea, whether it is save to destoy it here

      const ezGALRenderTargetView* pView = pDevice->GetRenderTargetView(pRenderTargetConfig->GetDescription().m_hColorTargets[i]);

      if (!pView->GetDescription().m_hTexture.IsInvalidated())
        pDevice->DestroyTexture(pView->GetDescription().m_hTexture);

      pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hColorTargets[i]);
    }

    if (!pRenderTargetConfig->GetDescription().m_hDepthStencilTarget.IsInvalidated())
      pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hDepthStencilTarget);

    if (!m_hRenderTargetConfig.IsInvalidated())
      pDevice->DestroyRenderTargetConfig(m_hRenderTargetConfig);
  }
  
  if (!m_hBackBufferTexture.IsInvalidated())
    pDevice->DestroyTexture(m_hBackBufferTexture);

  if (!m_hDepthStencilBufferTexture.IsInvalidated())
    pDevice->DestroyTexture(m_hDepthStencilBufferTexture);

  return EZ_SUCCESS;
}

void ezGALSwapChain::SetBackBufferObjects(ezGALRenderTargetConfigHandle hRenderTargetConfig, ezGALTextureHandle hBackBufferTexture, ezGALTextureHandle hDepthStencilBufferTexture)
{
  m_hRenderTargetConfig = hRenderTargetConfig;
  m_hBackBufferTexture = hBackBufferTexture;
  m_hBackBufferTexture = hDepthStencilBufferTexture;
}
