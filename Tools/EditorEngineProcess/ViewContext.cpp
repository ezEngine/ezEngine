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
#include <Foundation/Utilities/GraphicsUtils.h>

void ezViewContext::SetCamera(ezViewCameraMsgToEngine* pMsg)
{
  m_Camera.SetCameraMode((ezCamera::CameraMode) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
}

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
    if (!m_hPickingIdRT.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hPickingIdRT);
      m_hPickingIdRT.Invalidate();
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

    m_hPickingIdRT = pDevice->CreateTexture(tcd);

    tcd.m_Format = ezGALResourceFormat::DFloat;
    tcd.m_ResourceAccess.m_bReadBack = true;

    m_hPickingDepthRT = pDevice->CreateTexture(tcd);

    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = m_hPickingIdRT;
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

  const ezUInt32 uiWindowWidth = GetEditorWindow().m_uiWidth;
  const ezUInt32 uiWindowHeight = GetEditorWindow().m_uiHeight;
  const ezUInt32 uiIndex = (y * uiWindowWidth) + x;

  if (uiIndex > m_PickingResultsID.GetCount())
  {
    ezLog::Error("Picking position %u, %u is outside the available picking area of %u * %u", x, y, uiWindowWidth, uiWindowHeight);
  }
  else
  {
    const ezUInt32 uiComponentID = m_PickingResultsID[uiIndex];
    const float fDepth = m_PickingResultsDepth[uiIndex];

    res.m_ComponentGuid = ezEngineProcessGameState::GetInstance()->m_ComponentPickingMap.GetGuid(uiComponentID);
    res.m_OtherGuid = ezEngineProcessGameState::GetInstance()->m_OtherPickingMap.GetGuid(uiComponentID);

    if (res.m_ComponentGuid.IsValid())
    {
      ezComponentHandle hComponent = ezEngineProcessGameState::GetInstance()->m_ComponentMap.GetHandle(res.m_ComponentGuid);

      ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());

      // check whether the component is still valid
      ezComponent* pComponent = nullptr;
      if (pDocumentContext->m_pWorld->TryGetComponent<ezComponent>(hComponent, pComponent))
      {
        // if yes, fill out the parent game object guid
        res.m_ObjectGuid = ezEngineProcessGameState::GetInstance()->m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
        res.m_uiPartIndex = 0; /// TODO
      }
      else
      {
        res.m_ComponentGuid = ezUuid();
      }
    }

    /// \todo Add an enum that defines in which direction the window Y coordinate points ?
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - y, fDepth), res.m_vPickedPosition);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - y, 0), res.m_vPickingRayStartPosition);

    //ezLog::Info("Picked at %u, %u, %.2f, ID %u and GUID %s, Pos: %.2f | %.2f | %.2f, Start: %.2f | %.2f | %.2f", x, y, fDepth, uiComponentID, ezConversionUtils::ToString(res.m_ObjectGuid).GetData(), res.m_vPickedPosition.x, res.m_vPickedPosition.y, res.m_vPickedPosition.z, res.m_vPickingRayStartPosition.x, res.m_vPickingRayStartPosition.y, res.m_vPickingRayStartPosition.z);
  }

  SendViewMessage(&res);
}

void ezViewContext::Redraw()
{
  ezRenderLoop::AddMainView(m_pView);

  // download the picking information from the GPU
  if (GetEditorWindow().m_uiWidth != 0 && GetEditorWindow().m_uiHeight != 0)
  {
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hPickingIdRT);
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hPickingDepthRT);

    ezMat4 mProj, mView;

    m_Camera.GetProjectionMatrix(GetEditorWindow().m_uiWidth / GetEditorWindow().m_uiHeight, mProj);
    m_Camera.GetViewMatrix(mView);

    if (mProj.IsNaN())
      return;

    m_PickingInverseViewProjectionMatrix = (mProj * mView).GetInverse();

    m_PickingResultsID.Clear();
    m_PickingResultsID.SetCount(GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight);

    m_PickingResultsDepth.Clear();
    m_PickingResultsDepth.SetCount(GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight);

    ezGALSystemMemoryDescription MemDesc;
    MemDesc.m_uiRowPitch = 4 * GetEditorWindow().m_uiWidth;
    MemDesc.m_uiSlicePitch = 4 * GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight;

    MemDesc.m_pData = m_PickingResultsID.GetData();
    ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hPickingIdRT, &SysMemDescs);

    MemDesc.m_pData = m_PickingResultsDepth.GetData();
    ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hPickingDepthRT, &SysMemDescsDepth);
  }
}

