
#include <RendererDX11/PCH.h>
#include <RendererDX11/Basics.h>
#include <RendererDX11/Resources/RenderTargetConfigDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Logging/Log.h>


ezGALRenderTargetConfigDX11::ezGALRenderTargetConfigDX11(const ezGALRenderTargetConfigCreationDescription& Description)
  : ezGALRenderTargetConfig(Description)
{
  ezMemoryUtils::ZeroFill(m_pRenderTargetViews, EZ_GAL_MAX_RENDERTARGET_COUNT);
  m_pDepthStencilTargetView = nullptr;
}

ezGALRenderTargetConfigDX11::~ezGALRenderTargetConfigDX11()
{
}

ezResult ezGALRenderTargetConfigDX11::InitPlatform(ezGALDevice* pDevice)
{
  if(m_Description.IsValid())
  {
    for(ezUInt32 i = 0; i < m_Description.m_uiColorTargetCount; i++)
    {
      m_pRenderTargetViews[i] = static_cast<const ezGALRenderTargetViewDX11*>(pDevice->GetRenderTargetView(m_Description.m_hColorTargets[i]))->GetRenderTargetView();
      EZ_ASSERT(m_pRenderTargetViews[i] != nullptr, "Invalid handle given for rendertarget view!");
    }

    if(!m_Description.m_hDepthStencilTarget.IsInvalidated())
    {
      m_pDepthStencilTargetView = static_cast<const ezGALRenderTargetViewDX11*>(pDevice->GetRenderTargetView(m_Description.m_hDepthStencilTarget))->GetDepthStencilView();
      EZ_ASSERT(m_pDepthStencilTargetView != nullptr, "Invalid handle given for depth stencil view!");
    }

    return EZ_SUCCESS;
  }
  else
  {
    return EZ_FAILURE;
  }
}

ezResult ezGALRenderTargetConfigDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}


  /*
  ID3D11RenderTargetView* m_pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT];

  ID3D11DepthStencilView* m_pDepthStencilTargetView;*/