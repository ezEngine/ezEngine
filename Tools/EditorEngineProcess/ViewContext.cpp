#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameFoundation/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcess/PickingRenderPass.h>
#include <EditorEngineProcess/GameState.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>

void ezViewContext::SetCamera(const ezViewCameraMsgToEngine* pMsg)
{
  m_Camera.SetCameraMode((ezCamera::CameraMode) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
}

void ezViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezViewContext::SetupRenderTarget");

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

    m_hSwapChainRTV = pPrimarySwapChain->GetBackBufferRenderTargetView();
    m_hSwapChainDSV = pPrimarySwapChain->GetDepthStencilTargetView();
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
    m_hPickingIdRTV = pDevice->CreateRenderTargetView( rtvd );

    rtvd.m_hTexture = m_hPickingDepthRT;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;
    m_hPickingDepthDSV = pDevice->CreateRenderTargetView( rtvd );
  }

  // setup view
  {
    if (m_pView != nullptr)
    {
      ezRenderLoop::DeleteView(m_pView);
    }

    m_pView = ezRenderLoop::CreateView("Editor - View");

    ezGALRenderTagetSetup PickingRenderTargetSetup;
    PickingRenderTargetSetup.SetRenderTarget(0, m_hPickingIdRTV)
                            .SetDepthStencilTarget(m_hPickingDepthDSV);

    ezUniquePtr<ezPickingRenderPass> pRenderPass = EZ_DEFAULT_NEW(ezPickingRenderPass, PickingRenderTargetSetup);
    pRenderPass->m_Events.AddEventHandler(ezMakeDelegate(&ezViewContext::RenderPassEventHandler, this));

    ezGALRenderTagetSetup BackBufferRenderTargetSetup;
    BackBufferRenderTargetSetup.SetRenderTarget(0, m_hSwapChainRTV)
      .SetDepthStencilTarget(m_hSwapChainDSV);

    ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
    pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass, BackBufferRenderTargetSetup));
    pRenderPipeline->AddPass(std::move(pRenderPass));
    m_pView->SetRenderPipeline(std::move(pRenderPipeline));

    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));

    ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());
    m_pView->SetWorld(pDocumentContext->m_pWorld);
    m_pView->SetLogicCamera(&m_Camera);
  }
}

void ezViewContext::SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg, bool bSuperHighPriority)
{
  pViewMsg->m_DocumentGuid = GetDocumentGuid();
  pViewMsg->m_uiViewID = GetViewIndex();

  ezEngineProcessGameState::GetInstance()->ProcessCommunication().SendMessage(pViewMsg, bSuperHighPriority);
}

void ezViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;

  const ezUInt32 uiWindowWidth = GetEditorWindow().m_uiWidth;
  const ezUInt32 uiWindowHeight = GetEditorWindow().m_uiHeight;
  const ezUInt32 uiIndex = (y * uiWindowWidth) + x;

  if (uiIndex > m_PickingResultsID.GetCount())
  {
    //ezLog::Error("Picking position %u, %u is outside the available picking area of %u * %u", x, y, uiWindowWidth, uiWindowHeight);
  }
  else
  {
    const ezUInt32 uiComponentID = m_PickingResultsID[uiIndex];

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
    const float fDepth = m_PickingResultsDepth[uiIndex];
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - y, fDepth), res.m_vPickedPosition);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - y, 0), res.m_vPickingRayStartPosition);

    // compute the average normal at that position
    {
      float fOtherDepths[4] = { fDepth, fDepth, fDepth ,fDepth };
      ezVec3 vOtherPos[4];
      ezVec3 vNormals[4];

      if ((ezUInt32) x + 1 < uiWindowWidth)
        fOtherDepths[0] = m_PickingResultsDepth[(y * uiWindowWidth) + x + 1];
      if (x > 0)
        fOtherDepths[1] = m_PickingResultsDepth[(y * uiWindowWidth) + x - 1];
      if ((ezUInt32)y + 1 < uiWindowHeight)
        fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * uiWindowWidth) + x];
      if (y > 0)
        fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * uiWindowWidth) + x];

      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x + 1, uiWindowHeight - y, fOtherDepths[0]), vOtherPos[0]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x - 1, uiWindowHeight - y, fOtherDepths[1]), vOtherPos[1]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - (y + 1), fOtherDepths[2]), vOtherPos[2]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, uiWindowHeight - (y - 1), fOtherDepths[3]), vOtherPos[3]);

      vNormals[0] = ezPlane(res.m_vPickedPosition, vOtherPos[0], vOtherPos[2]).m_vNormal;
      vNormals[1] = ezPlane(res.m_vPickedPosition, vOtherPos[2], vOtherPos[1]).m_vNormal;
      vNormals[2] = ezPlane(res.m_vPickedPosition, vOtherPos[1], vOtherPos[3]).m_vNormal;
      vNormals[3] = ezPlane(res.m_vPickedPosition, vOtherPos[3], vOtherPos[0]).m_vNormal;

      res.m_vPickedNormal = (vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3]).GetNormalized();
    }

    ezLog::Info("Picked Normal: %.2f | %.2f | %.2f", res.m_vPickedNormal.x, res.m_vPickedNormal.y, res.m_vPickedNormal.z);
  }

  SendViewMessage(&res, true);
}

void ezViewContext::Redraw()
{
  ezRenderLoop::AddMainView(m_pView);

}

void ezViewContext::RenderPassEventHandler(const ezPickingRenderPass::Event& e)
{
  if (e.m_Type == ezPickingRenderPass::Event::Type::AfterOpaque)
  {
    // download the picking information from the GPU
    if (GetEditorWindow().m_uiWidth != 0 && GetEditorWindow().m_uiHeight != 0)
    {
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hPickingDepthRT);

      ezMat4 mProj, mView;

      m_Camera.GetProjectionMatrix((float)GetEditorWindow().m_uiWidth / GetEditorWindow().m_uiHeight, mProj);
      m_Camera.GetViewMatrix(mView);

      if (mProj.IsNaN())
        return;

      m_PickingInverseViewProjectionMatrix = (mProj * mView).GetInverse();

      m_PickingResultsDepth.Clear();
      m_PickingResultsDepth.SetCount(GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight);

      ezGALSystemMemoryDescription MemDesc;
      MemDesc.m_uiRowPitch = 4 * GetEditorWindow().m_uiWidth;
      MemDesc.m_uiSlicePitch = 4 * GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight;

      MemDesc.m_pData = m_PickingResultsDepth.GetData();
      ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hPickingDepthRT, &SysMemDescsDepth);
    }
  }

  if (e.m_Type == ezPickingRenderPass::Event::Type::EndOfFrame)
  {

    // download the picking information from the GPU
    if (GetEditorWindow().m_uiWidth != 0 && GetEditorWindow().m_uiHeight != 0)
    {
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hPickingIdRT);

      ezMat4 mProj, mView;

      m_Camera.GetProjectionMatrix((float)GetEditorWindow().m_uiWidth / GetEditorWindow().m_uiHeight, mProj);
      m_Camera.GetViewMatrix(mView);

      if (mProj.IsNaN())
        return;

      m_PickingInverseViewProjectionMatrix = (mProj * mView).GetInverse();

      m_PickingResultsID.Clear();
      m_PickingResultsID.SetCount(GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight);

      ezGALSystemMemoryDescription MemDesc;
      MemDesc.m_uiRowPitch = 4 * GetEditorWindow().m_uiWidth;
      MemDesc.m_uiSlicePitch = 4 * GetEditorWindow().m_uiWidth * GetEditorWindow().m_uiHeight;

      MemDesc.m_pData = m_PickingResultsID.GetData();
      ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hPickingIdRT, &SysMemDescs);
    }
  }
}

