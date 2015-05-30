#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <GameFoundation/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcess/PickingRenderPass.h>
#include <EditorEngineProcess/GameState.h>
#include <RendererCore/RenderContext/RenderContext.h>

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

    //m_PickingRenderTargetDT.EnableDataTransfer("Picking RT");
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
    pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezPickingRenderPass, m_hPickingRenderTargetCfg));
    m_pView->SetRenderPipeline(pRenderPipeline);

    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));

    ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());
    m_pView->SetWorld(pDocumentContext->m_pWorld);
    m_pView->SetLogicCamera(&m_Camera);
  }
}

void ezViewContext::SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentGuid();
  pViewMsg->m_uiViewID = GetViewIndex();

  ezEngineProcessGameState::GetInstance()->ProcessCommunication().SendMessage(pViewMsg);
}

void ezViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;

  const ezUInt32 uiIndex = (y * GetEditorWindow().m_uiWidth) + x;

  if (uiIndex > m_PickingResultsComponentID.GetCount())
  {
    ezLog::Error("Picking position %u, %u is outside the available picking area of %u * %u", x, y, GetEditorWindow().m_uiWidth, GetEditorWindow().m_uiHeight);
  }
  else
  {
    const ezUInt32 uiComponentID = m_PickingResultsComponentID[uiIndex];

    //ezComponentHandle hComponent()

    /// \todo Passing around handles as ints is tedious
    const ezGameObjectHandle hObject = ezGameObjectHandle(ezGameObjectId(uiComponentID));

    res.m_ObjectGuid = ezEngineProcessGameState::GetInstance()->m_GameObjectMap.GetGuid(hObject);

    ezLog::Info("Picked component at %u, %u with ID %u and GUID %s", x, y, uiComponentID, ezConversionUtils::ToString(res.m_ObjectGuid).GetData());
  }

  SendViewMessage(&res);
}

void ezViewContext::Redraw()
{
  ezRenderLoop::AddMainView(m_pView);

  //if (m_PickingRenderTargetDT.IsTransferRequested())
  {
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hPickingRT);

    m_PickingResultsComponentID.Clear();
    m_PickingResultsComponentID.SetCount(GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight);

    ezGALSystemMemoryDescription MemDesc;
    MemDesc.m_pData = m_PickingResultsComponentID.GetData();
    MemDesc.m_uiRowPitch = 4 * GetEditorWindow().m_uiWidth;
    MemDesc.m_uiSlicePitch = 4 * GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight;

    ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hPickingRT, &SysMemDescs);

    //for (ezUInt32 i = 0; i < ImageContent.GetCount(); i += 4)
    //{
    //  ezMath::Swap(ImageContent[i + 0], ImageContent[i + 2]);
    //}

    //ezDataTransferObject DataObject(m_PickingRenderTargetDT, "Picking IDs", "image/rgba8", "rgba");
    //DataObject.GetWriter() << GetEditorWindow().m_uiWidth;
    //DataObject.GetWriter() << GetEditorWindow().m_uiHeight;
    //DataObject.GetWriter().WriteBytes(ImageContent.GetData(), ImageContent.GetCount());

    //DataObject.Transmit();
  }
}

