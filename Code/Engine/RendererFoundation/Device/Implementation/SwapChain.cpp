
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
      /// \todo The texture was a memory leak, but I have no idea, whether it is save to destroy it here

      const ezGALRenderTargetView* pView = pDevice->GetRenderTargetView(pRenderTargetConfig->GetDescription().m_hColorTargets[i]);

      if (!pView->GetDescription().m_hTexture.IsInvalidated())
      {
        if (pView->GetDescription().m_hTexture == m_hBackBufferTexture)
        {
          /// \todo this is a workaround to prevent double-free, not sure this is correct
          m_hBackBufferTexture.Invalidate();
        }

        pDevice->DestroyTexture(pView->GetDescription().m_hTexture);
      }

      pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hColorTargets[i]);
    }

    if (!pRenderTargetConfig->GetDescription().m_hDepthStencilTarget.IsInvalidated())
    {
      /// \todo Are we supposed to do this here ? pRenderTargetConfig is const ...

      pDevice->DestroyRenderTargetView(pRenderTargetConfig->GetDescription().m_hDepthStencilTarget);
      //pRenderTargetConfig->GetDescription().m_hDepthStencilTarget.Invalidate();
    }

    if (!m_hRenderTargetConfig.IsInvalidated())
    {
      pDevice->DestroyRenderTargetConfig(m_hRenderTargetConfig);
      m_hRenderTargetConfig.Invalidate();
    }
  }
  
  if (!m_hBackBufferTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hBackBufferTexture);
    m_hBackBufferTexture.Invalidate();
  }

  if (!m_hDepthStencilBufferTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hDepthStencilBufferTexture);
    m_hDepthStencilBufferTexture.Invalidate();
  }

  return EZ_SUCCESS;
}

void ezGALSwapChain::SetBackBufferObjects(ezGALRenderTargetConfigHandle hRenderTargetConfig, ezGALTextureHandle hBackBufferTexture, ezGALTextureHandle hDepthStencilBufferTexture)
{
  m_hRenderTargetConfig = hRenderTargetConfig;
  m_hBackBufferTexture = hBackBufferTexture;
  m_hDepthStencilBufferTexture = hDepthStencilBufferTexture;
}
