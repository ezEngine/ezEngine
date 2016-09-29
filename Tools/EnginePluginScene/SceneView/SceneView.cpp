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

ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext)
  : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
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
      m_pView->SetRenderPassProperty("EditorPickingPass", "Active", pMsg2->m_bUpdatePickingData);
      m_pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
      m_pView->SetRenderPassProperty("EditorPickingPass.ezGizmoRenderer", "Enabled", pMsg2->m_bEnablePickingSelected);
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
  if (m_pView)
  {
    m_pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    m_pView->SetRenderPassProperty("EditorRenderPass", "ViewRenderMode", (ezUInt8)ezViewRenderMode::Default);
    m_pView->SetRenderPassProperty("EditorPickingPass", "ViewRenderMode", (ezUInt8)ezViewRenderMode::Default);
    m_pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
  }

  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, ezVec3(1.0f, 1.0f, -1.0f));
}

void ezSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  const ezTag* tagNoOrtho = ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

  if (m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedHeight ||
      m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    m_pView->m_ExcludeTags.Set(*tagNoOrtho);
  }
  else
  {
    m_pView->m_ExcludeTags.Remove(*tagNoOrtho);
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
    m_pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", m_pSceneContext->GetRenderShapeIcons());
  }
}

ezView* ezSceneViewContext::CreateView()
{
  ezView* pView = ezRenderLoop::CreateView("Editor - View");

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  pView->SetRenderPassProperty("EditorPickingPass", "SceneContext", m_pSceneContext);
  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", m_pSceneContext);
  pView->SetExtractorProperty("EditorShapeIconsExtractor", "SceneContext", m_pSceneContext);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetLogicCamera(&m_Camera);

  auto& tagReg = ezTagRegistry::GetGlobalRegistry();
  const ezTag* tagHidden = tagReg.RegisterTag("EditorHidden");

  pView->m_ExcludeTags.Set(*tagHidden);
  return pView;
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
