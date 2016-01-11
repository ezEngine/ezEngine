#include <PCH.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameFoundation/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameFoundation/GameApplication.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <SceneContext/SceneContext.h>
#include <Core/Scene/Scene.h>



ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext) : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_pView = nullptr;
  m_pEditorRenderPass = nullptr;
  m_pPickingRenderPass = nullptr;
  m_bUpdatePickingData = true;
}

ezSceneViewContext::~ezSceneViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

void ezSceneViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
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

  // setup view
  {
    if (m_pView == nullptr)
    {
      CreateView();
    }

    ezGALRenderTagetSetup BackBufferRenderTargetSetup;
    BackBufferRenderTargetSetup.SetRenderTarget(0, m_hSwapChainRTV)
      .SetDepthStencilTarget(m_hSwapChainDSV);
    m_pView->SetRenderTargetSetup(BackBufferRenderTargetSetup);
    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
  }
}

void ezSceneViewContext::Redraw()
{
  ezTag tagNoOrtho;
  ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode", &tagNoOrtho);

  if (m_pView->GetRenderCamera()->GetCameraMode() == ezCamera::OrthoFixedHeight ||
      m_pView->GetRenderCamera()->GetCameraMode() == ezCamera::OrthoFixedWidth)
  {
    m_pView->m_ExcludeTags.Set(tagNoOrtho);
  }
  else
  {
    m_pView->m_ExcludeTags.Remove(tagNoOrtho);
  }

  ezRenderLoop::AddMainView(m_pView);
}

void ezSceneViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  m_Camera.SetCameraMode((ezCamera::CameraMode) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  if (!m_pEditorRenderPass)
    return;

  m_pEditorRenderPass->m_bRenderSelectionOverlay = m_pSceneContext->GetRenderSelectionOverlay();
  m_pEditorRenderPass->m_ViewRenderMode = static_cast<ezViewRenderMode::Enum>(pMsg->m_uiRenderMode);
  m_pPickingRenderPass->m_ViewRenderMode = static_cast<ezViewRenderMode::Enum>(pMsg->m_uiRenderMode);
}

void ezSceneViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;

  const ezUInt32 uiWindowWidth = GetEditorWindow().m_uiWidth;
  const ezUInt32 uiWindowHeight = GetEditorWindow().m_uiHeight;
  const ezUInt32 uiIndex = (y * uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsID.GetCount())
  {
    //ezLog::Error("Picking position %u, %u is outside the available picking area of %u * %u", x, y, uiWindowWidth, uiWindowHeight);
  }
  else
  {
    const ezUInt32 uiComponentID = (m_PickingResultsID[uiIndex] & 0x00FFFFFF);
    const ezUInt32 uiPartIndex = (m_PickingResultsID[uiIndex] >> 24);

    res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
    res.m_OtherGuid = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

    if (res.m_ComponentGuid.IsValid())
    {
      ezComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

      ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

      // check whether the component is still valid
      ezComponent* pComponent = nullptr;
      if (pDocumentContext->GetScene()->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
      {
        // if yes, fill out the parent game object guid
        res.m_ObjectGuid = GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
        res.m_uiPartIndex = uiPartIndex;
      }
      else
      {
        res.m_ComponentGuid = ezUuid();
      }
    }

    /// \todo Add an enum that defines in which direction the window Y coordinate points ?
    const float fDepth = m_PickingResultsDepth[uiIndex];
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, (float)(uiWindowHeight - y), fDepth), res.m_vPickedPosition);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, (float)(uiWindowHeight - y), 0), res.m_vPickingRayStartPosition);

    // compute the average normal at that position
    {
      float fOtherDepths[4] = { fDepth, fDepth, fDepth, fDepth };
      ezVec3 vOtherPos[4];
      ezVec3 vNormals[4];

      if ((ezUInt32)x + 1 < uiWindowWidth)
        fOtherDepths[0] = m_PickingResultsDepth[(y * uiWindowWidth) + x + 1];
      if (x > 0)
        fOtherDepths[1] = m_PickingResultsDepth[(y * uiWindowWidth) + x - 1];
      if ((ezUInt32)y + 1 < uiWindowHeight)
        fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * uiWindowWidth) + x];
      if (y > 0)
        fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * uiWindowWidth) + x];

      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3((float)(x + 1), (float)(uiWindowHeight - y), fOtherDepths[0]), vOtherPos[0]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3((float)(x - 1), (float)(uiWindowHeight - y), fOtherDepths[1]), vOtherPos[1]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, (float)(uiWindowHeight - (y + 1)), fOtherDepths[2]), vOtherPos[2]);
      ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, uiWindowWidth, uiWindowHeight, ezVec3(x, (float)(uiWindowHeight - (y - 1)), fOtherDepths[3]), vOtherPos[3]);

      vNormals[0] = ezPlane(res.m_vPickedPosition, vOtherPos[0], vOtherPos[2]).m_vNormal;
      vNormals[1] = ezPlane(res.m_vPickedPosition, vOtherPos[2], vOtherPos[1]).m_vNormal;
      vNormals[2] = ezPlane(res.m_vPickedPosition, vOtherPos[1], vOtherPos[3]).m_vNormal;
      vNormals[3] = ezPlane(res.m_vPickedPosition, vOtherPos[3], vOtherPos[0]).m_vNormal;

      res.m_vPickedNormal = (vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3]).GetNormalized();
    }

    //ezLog::Info("Picked Normal: %.2f | %.2f | %.2f", res.m_vPickedNormal.x, res.m_vPickedNormal.y, res.m_vPickedNormal.z);
  }

  SendViewMessage(&res);
}

void ezSceneViewContext::SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void ezSceneViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    if (m_pPickingRenderPass)
    {
      m_pPickingRenderPass->SetEnabled(pMsg2->m_bUpdatePickingData);
      m_pPickingRenderPass->SetPickSelected(pMsg2->m_bEnablePickingSelected);
    }

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
      SetupRenderTarget(reinterpret_cast<HWND>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
      Redraw();
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {


    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }


}

void ezSceneViewContext::RenderPassEventHandler(const ezPickingRenderPass::Event& e)
{
  if (e.m_Type == ezPickingRenderPass::Event::Type::AfterOpaque)
  {
    // download the picking information from the GPU
    if (GetEditorWindow().m_uiWidth != 0 && GetEditorWindow().m_uiHeight != 0)
    {
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_pPickingRenderPass->GetPickingDepthRT());

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
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_pPickingRenderPass->GetPickingDepthRT(), &SysMemDescsDepth);
    }
  }

  if (e.m_Type == ezPickingRenderPass::Event::Type::EndOfFrame)
  {

    // download the picking information from the GPU
    if (GetEditorWindow().m_uiWidth != 0 && GetEditorWindow().m_uiHeight != 0)
    {
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_pPickingRenderPass->GetPickingIdRT());

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
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_pPickingRenderPass->GetPickingIdRT(), &SysMemDescs);
    }
  }
}

void ezSceneViewContext::CreateView()
{
  m_pView = ezRenderLoop::CreateView("Editor - View");

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  {
    ezUniquePtr<ezRenderPipelinePass> pPass = EZ_DEFAULT_NEW(ezEditorRenderPass);
    m_pEditorRenderPass = static_cast<ezEditorRenderPass*>(pPass.Borrow());
    m_pEditorRenderPass->SetSceneContext(m_pSceneContext);
    pRenderPipeline->AddPass(std::move(pPass));
  }

  {
    ezUniquePtr<ezRenderPipelinePass> pPass = EZ_DEFAULT_NEW(ezPickingRenderPass);
    m_pPickingRenderPass = static_cast<ezPickingRenderPass*>(pPass.Borrow());
    m_pPickingRenderPass->SetSceneContext(m_pSceneContext);
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezRenderPipelinePass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = static_cast<ezTargetPass*>(pPass.Borrow());
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(m_pEditorRenderPass, "Color", pTargetPass, "Color0"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(m_pEditorRenderPass, "DepthStencil", pTargetPass, "DepthStencil"), "Connect failed!");

  m_pPickingRenderPass->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneViewContext::RenderPassEventHandler, this));

  ezUniquePtr<ezSelectedObjectsExtractor> pExtractorSelection = EZ_DEFAULT_NEW(ezSelectedObjectsExtractor);
  m_pSelectionExtractor = pExtractorSelection.Borrow();
  m_pSelectionExtractor->m_pSelection = &m_pSceneContext->GetSelectionWithChildren();

  ezUniquePtr<ezCallDelegateExtractor> pExtractorShapeIcons = EZ_DEFAULT_NEW(ezCallDelegateExtractor);
  pExtractorShapeIcons->m_Delegate = ezMakeDelegate(&ezSceneContext::GenerateShapeIconMesh, m_pSceneContext);

  pRenderPipeline->AddExtractor(std::move(pExtractorSelection));
  pRenderPipeline->AddExtractor(std::move(pExtractorShapeIcons));
  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));

  m_pView->SetRenderPipeline(std::move(pRenderPipeline));

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  m_pView->SetWorld(pDocumentContext->GetScene()->GetWorld());
  m_pView->SetLogicCamera(&m_Camera);

  auto& tagReg = ezTagRegistry::GetGlobalRegistry();
  ezTag tagHidden;
  tagReg.RegisterTag("EditorHidden", &tagHidden);

  ezTag tagSel;
  ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorSelected", &tagSel);

  m_pView->m_ExcludeTags.Set(tagHidden);
  m_pView->m_ExcludeTags.Set(tagSel);
}
