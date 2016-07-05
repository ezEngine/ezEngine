#include <PCH.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <SceneContext/SceneContext.h>
#include <RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>

const char* s_szEditorPipelineID = "EditorRenderPipeline";

ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext) : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1,1,1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezSceneViewContext::~ezSceneViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

void ezSceneViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  ezEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    if (m_pView)
    {
      m_pView->SetRenderPassProperty("EditorPickingPass", "Enable", pMsg2->m_bUpdatePickingData);
      m_pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {
    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
}

bool ezSceneViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  bool bChanged = false;
  ezVec3 vCameraPos = m_Camera.GetCenterPosition();
  ezVec3 vCenterPos = bounds.GetSphere().m_vCenter;
  
  float fFov = 45.0f;
  const float fDist = bounds.GetSphere().m_fRadius / ezMath::Sin(ezAngle::Degree(fFov / 2));
  ezVec3 vDir(1.0f, 1.0f, -1.0f);
  vDir.Normalize();
  ezVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    m_Camera.LookAt(vNewCameraPos, vCenterPos, ezVec3(0.0f, 0.0f, 1.0f));
    return false;
  }

  return true;
}

void ezSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  ezTag tagNoOrtho;
  ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode", &tagNoOrtho);

  if (m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedHeight ||
      m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    m_pView->m_ExcludeTags.Set(tagNoOrtho);
  }
  else
  {
    m_pView->m_ExcludeTags.Remove(tagNoOrtho);
  }

  m_pView->SetRenderPassProperty("SimplePass.ezGizmoRenderer", "HighlightID", GetDocumentContext()->m_Context.m_uiHighlightID);

  ezEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

void ezSceneViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  if (m_pView)
  {
    m_pView->SetRenderPassProperty("EditorSelectionPass", "Active", m_pSceneContext->GetRenderSelectionOverlay());
    m_pView->SetRenderPassProperty("EditorRenderPass", "ViewRenderMode", pMsg->m_uiRenderMode);
    m_pView->SetRenderPassProperty("EditorPickingPass", "ViewRenderMode", pMsg->m_uiRenderMode);
  }
}

ezView* ezSceneViewContext::CreateView()
{
  ezView* pView = ezRenderLoop::CreateView("Editor - View");

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  pView->SetRenderPassProperty("EditorPickingPass", "SceneContext", m_pSceneContext);
  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", m_pSceneContext);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetLogicCamera(&m_Camera);

  auto& tagReg = ezTagRegistry::GetGlobalRegistry();
  ezTag tagHidden;
  tagReg.RegisterTag("EditorHidden", &tagHidden);

  pView->m_ExcludeTags.Set(tagHidden);
  return pView;
}

bool ezSceneViewContext::IsDefaultRenderPipeline(ezRenderPipelineResourceHandle hPipeline)
{
  ezResourceLock<ezRenderPipelineResource> pPipeline(hPipeline, ezResourceAcquireMode::NoFallback);
  return pPipeline->GetResourceID() == s_szEditorPipelineID;
}

ezRenderPipelineResourceHandle ezSceneViewContext::CreateDefaultRenderPipeline()
{
  auto hPipe = ezResourceManager::GetExistingResource<ezRenderPipelineResource>(s_szEditorPipelineID);
  if (hPipe.IsValid())
  {
    return hPipe;
  }

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezEditorRenderPass* pEditorRenderPass = nullptr;
  {
    ezUniquePtr<ezEditorRenderPass> pPass = EZ_DEFAULT_NEW(ezEditorRenderPass);
    pEditorRenderPass = pPass.Borrow();
    pEditorRenderPass->SetName("EditorRenderPass");
    pEditorRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pEditorRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezLightGatheringRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezPickingRenderPass* pPickingRenderPass = nullptr;
  {
    ezUniquePtr<ezPickingRenderPass> pPass = EZ_DEFAULT_NEW(ezPickingRenderPass);
    pPickingRenderPass = pPass.Borrow();
    pPickingRenderPass->SetName("EditorPickingPass");
    pPickingRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pPickingRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezGizmoRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezSelectionHighlightPass* pSelectionHighlightPass = nullptr;
  {
    ezUniquePtr<ezSelectionHighlightPass> pPass = EZ_DEFAULT_NEW(ezSelectionHighlightPass);
    pSelectionHighlightPass = pPass.Borrow();
    pSelectionHighlightPass->SetName("EditorSelectionPass");
    pSelectionHighlightPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezSimpleRenderPass* pSimplePass = nullptr;
  {
    ezUniquePtr<ezSimpleRenderPass> pPass = EZ_DEFAULT_NEW(ezSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pSimplePass->SetName("SimplePass");
    pSimplePass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pSimplePass->AddRenderer(EZ_DEFAULT_NEW(ezGizmoRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezTargetPass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(pEditorRenderPass, "Color", pSelectionHighlightPass, "Color"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pEditorRenderPass, "DepthStencil", pSelectionHighlightPass, "DepthStencil"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pEditorRenderPass, "DepthStencil", pSimplePass, "DepthStencil"), "Connect failed!");
  
  EZ_VERIFY(pRenderPipeline->Connect(pSelectionHighlightPass, "Color", pSimplePass, "Color"), "Connect failed!");  

  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "DepthStencil", pTargetPass, "DepthStencil"), "Connect failed!");

  ezUniquePtr<ezEditorSelectedObjectsExtractor> pExtractorSelection = EZ_DEFAULT_NEW(ezEditorSelectedObjectsExtractor);
  pExtractorSelection->SetName("EditorSelectedObjectsExtractor");

  pRenderPipeline->AddExtractor(std::move(pExtractorSelection));
  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));

  ezRenderPipelineResourceDescriptor desc;
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return ezResourceManager::CreateResource<ezRenderPipelineResource>(s_szEditorPipelineID, desc);

}

void ezSceneViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;

  if (m_pView != nullptr && m_pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickingMatrix"))
  {
    m_pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", ezVec2(x, y));
    ezVariant varMat = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingMatrix");

    if (varMat.IsA<ezMat4>())
    {
      const ezMat4 mInverseMat = varMat.Get<ezMat4>();
      const ezUInt32 uiPickingID = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingID").ConvertTo<ezUInt32>();
      const float fPickingDepth = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingDepth").ConvertTo<float>();
      res.m_vPickedNormal = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingNormal").ConvertTo<ezVec3>();
      res.m_vPickingRayStartPosition = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<ezVec3>();
      res.m_vPickedPosition = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingPosition").ConvertTo<ezVec3>();

      EZ_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

      const ezUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
      const ezUInt32 uiPartIndex = (uiPickingID >> 24);

      res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
      res.m_OtherGuid = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

      if (res.m_ComponentGuid.IsValid())
      {
        ezComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

        ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

        // check whether the component is still valid
        ezComponent* pComponent = nullptr;
        if (pDocumentContext->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
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
    }
  }

  SendViewMessage(&res);
}
