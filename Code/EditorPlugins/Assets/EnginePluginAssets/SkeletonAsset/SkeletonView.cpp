#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezSkeletonViewContext::ezSkeletonViewContext(ezSkeletonContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezSkeletonViewContext::~ezSkeletonViewContext() = default;

bool ezSkeletonViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


void ezSkeletonViewContext::Redraw(bool bRenderEditorGizmos)
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    const ezTag& tagNoOrtho = ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    EZ_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<ezGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  ezEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

ezViewHandle ezSkeletonViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Skeleton Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezSkeletonViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::AllowLoadingFallback);

    ezUInt32 uiNumJoints = pSkeleton->GetDescriptor().m_Skeleton.GetJointCount();

    ezStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 10), ezColor::White, 16, ezDebugRenderer::HorizontalAlignment::Left,
      ezDebugRenderer::VerticalAlignment::Bottom);
  }
}

void ezSkeletonViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {
    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", true);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
    }

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else
  {
    ezEngineProcessViewContext::HandleViewMessage(pMsg);
  }
}

void ezSkeletonViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  ezViewPickingResultMsgToEditor res;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView) && pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickingMatrix"))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", ezVec2(x, y));
    ezVariant varMat = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingMatrix");

    if (varMat.IsA<ezMat4>())
    {
      const ezUInt32 uiPickingID = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingID").ConvertTo<ezUInt32>();
      // const float fPickingDepth = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingDepth").ConvertTo<float>();
      res.m_vPickedNormal = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingNormal").ConvertTo<ezVec3>();
      res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<ezVec3>();
      res.m_vPickedPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingPosition").ConvertTo<ezVec3>();

      EZ_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

      const ezUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
      const ezUInt32 uiPartIndex = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

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
