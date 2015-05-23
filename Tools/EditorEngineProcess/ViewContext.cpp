#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <GameFoundation/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

ezMeshBufferResourceHandle CreateTranslateGizmoMesh();

void ezViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  if (GetEditorWindow().m_hWnd != 0)
  {
    if (GetEditorWindow().m_uiWidth == uiWidth && GetEditorWindow().m_uiHeight == uiHeight)
      return;

    m_hPrimarySwapChain.Invalidate();
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  GetEditorWindow().m_hWnd = hWnd;
  GetEditorWindow().m_uiWidth = uiWidth;
  GetEditorWindow().m_uiHeight = uiHeight;

  ezLog::Debug("Creating Swapchain with size %u * %u", uiWidth, uiHeight);

  {
    m_hPrimarySwapChain = static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->AddWindow(&GetEditorWindow());
    const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(m_hPrimarySwapChain);
    EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

    m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
    EZ_ASSERT_DEV(!m_hBBRT.IsInvalidated(), "Failed to init render target");
  }

  // setup view
  {
    if (m_pView != nullptr)
    {
      ezRenderPipeline* pRenderPipeline = m_pView->GetRenderPipeline();
      EZ_DEFAULT_DELETE(pRenderPipeline);
      EZ_DEFAULT_DELETE(m_pView);
    }

    m_pView = ezRenderLoop::CreateView("Editor - View");

    ezRenderPipeline* pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
    pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass, m_hBBRT));
    m_pView->SetRenderPipeline(pRenderPipeline);

    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));

    ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());
    m_pView->SetWorld(pDocumentContext->m_pWorld);
    m_pView->SetLogicCamera(&m_Camera);
  }

  // Create render target for picking
  {
    if (!m_hPickingRT.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hPickingRT);
      m_hPickingRT.Invalidate();
    }

    if (!m_hPickingDepthRT.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hPickingDepthRT);
      m_hPickingDepthRT.Invalidate();
    }

    if (!m_hPickingRenderTargetCfg.IsInvalidated())
    {
      pDevice->DestroyRenderTargetConfig(m_hPickingRenderTargetCfg);
      m_hPickingRenderTargetCfg.Invalidate();
    }

    ezGALTextureCreationDescription tcd;
    tcd.m_bAllowDynamicMipGeneration = false;
    tcd.m_bAllowShaderResourceView = false;
    tcd.m_bAllowUAV = false;
    tcd.m_bCreateRenderTarget = true;
    tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    tcd.m_ResourceAccess.m_bReadBack = true;
    tcd.m_Type = ezGALTextureType::Texture2D;
    tcd.m_uiWidth = uiWidth;
    tcd.m_uiHeight = uiHeight;

    m_hPickingRT = pDevice->CreateTexture(tcd);

    tcd.m_Format = ezGALResourceFormat::D24S8;
    tcd.m_ResourceAccess.m_bReadBack = false;

    m_hPickingDepthRT = pDevice->CreateTexture(tcd);

    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = m_hPickingRT;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;
    ezGALRenderTargetViewHandle hRTVcol = pDevice->CreateRenderTargetView(rtvd);

    rtvd.m_hTexture = m_hPickingDepthRT;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;
    ezGALRenderTargetViewHandle hRTVdepth = pDevice->CreateRenderTargetView(rtvd);

    ezGALRenderTargetConfigCreationDescription rtd;
    rtd.m_bHardwareBackBuffer = false;
    rtd.m_hColorTargets[0] = hRTVcol;
    rtd.m_hDepthStencilTarget = hRTVdepth;
    rtd.m_uiColorTargetCount = 1;

    m_hPickingRenderTargetCfg = pDevice->CreateRenderTargetConfig(rtd);

    m_PickingRenderTargetDT.EnableDataTransfer("Picking RT");
  }

}

void ezViewContext::Redraw()
{
  ezRenderLoop::AddMainView(m_pView);
}

