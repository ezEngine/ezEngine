#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext)
  : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));

  m_CullingCamera = m_Camera;
}

ezSceneViewContext::~ezSceneViewContext() = default;

void ezSceneViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  ezEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", pMsg2->m_bUpdatePickingData);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
      pView->SetRenderPassProperty("EditorPickingPass", "PickTransparent", pMsg2->m_bEnablePickTransparent);
    }

    if (pMsg2->m_iCameraMode == ezCameraMode::PerspectiveFixedFovX || pMsg2->m_iCameraMode == ezCameraMode::PerspectiveFixedFovY)
    {
      if (!m_pSceneContext->IsPlayTheGameActive())
      {
        if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
        {
          pSoundInterface->SetListener(-1, pMsg2->m_vPosition, pMsg2->m_vDirForwards, pMsg2->m_vDirUp, ezVec3::MakeZero());
        }
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {
    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewMarqueePickingMsgToEngine>())
  {
    const ezViewMarqueePickingMsgToEngine* pMsg2 = static_cast<const ezViewMarqueePickingMsgToEngine*>(pMsg);

    MarqueePickObjects(pMsg2);
  }
}

void ezSceneViewContext::SetupRenderTarget(ezGALSwapChainHandle hSwapChain, const ezGALRenderTargets* pRenderTargets, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  ezEngineProcessViewContext::SetupRenderTarget(hSwapChain, pRenderTargets, uiWidth, uiHeight);
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    ezTagSet& excludeTags = pView->m_ExcludeTags;
    const ezArrayPtr<const ezTag> addTags = m_pSceneContext->GetInvisibleLayerTags();
    for (const ezTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

bool ezSceneViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewRenderMode(ezViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
  }

  EZ_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  const ezCameraComponentManager* pCamMan = m_pSceneContext->GetWorld()->GetComponentManager<ezCameraComponentManager>();
  if (pCamMan)
  {
    for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
    {
      const ezCameraComponent* pCamComp = it;

      if (pCamComp->GetUsageHint() == ezCameraUsageHint::Thumbnail)
      {
        m_Camera.LookAt(pCamComp->GetOwner()->GetGlobalPosition(), pCamComp->GetOwner()->GetGlobalPosition() + pCamComp->GetOwner()->GetGlobalDirForwards(), pCamComp->GetOwner()->GetGlobalDirUp());

        m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 70.0f, 0.1f, 100.0f);

        m_CullingCamera = m_Camera;
        return true;
      }
    }
  }

  bool bResult = !FocusCameraOnObject(m_Camera, bounds, 70.0f, -ezVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
}

void ezSceneViewContext::SetInvisibleLayerTags(const ezArrayPtr<ezTag> removeTags, const ezArrayPtr<ezTag> addTags)
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    ezTagSet& excludeTags = pView->m_ExcludeTags;
    for (const ezTag& removeTag : removeTags)
    {
      excludeTags.Remove(removeTag);
    }
    for (const ezTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

void ezSceneViewContext::Redraw(bool bRenderEditorGizmos)
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

void ezSceneViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  ezView* pView = nullptr;
  ezRenderWorld::TryGetView(m_hView, pView);

  bool bDebugCulling = false;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  bDebugCulling = ezRenderPipeline::cvar_SpatialCullingVis;
#endif

  if (bDebugCulling && pView != nullptr)
  {
    if (const ezCameraComponentManager* pCameraManager = pView->GetWorld()->GetComponentManager<ezCameraComponentManager>())
    {
      if (const ezCameraComponent* pCameraComponent = pCameraManager->GetCameraByUsageHint(ezCameraUsageHint::Culling))
      {
        const ezGameObject* pOwner = pCameraComponent->GetOwner();
        ezVec3 vPosition = pOwner->GetGlobalPosition();
        ezVec3 vForward = pOwner->GetGlobalDirForwards();
        ezVec3 vUp = pOwner->GetGlobalDirUp();

        m_CullingCamera.LookAt(vPosition, vPosition + vForward, vUp);

        auto cameraMode = pCameraComponent->GetCameraMode();
        float fFovOrDim = pCameraComponent->GetFieldOfView();
        if (cameraMode == ezCameraMode::OrthoFixedWidth || cameraMode == ezCameraMode::OrthoFixedHeight)
        {
          fFovOrDim = pCameraComponent->GetOrthoDimension();
        }

        const float fNearPlane = pCameraComponent->GetNearPlane();
        const float fFarPlane = pCameraComponent->GetFarPlane();
        m_CullingCamera.SetCameraMode(cameraMode, fFovOrDim, fNearPlane, ezMath::Max(fNearPlane + 0.00001f, fFarPlane));
      }
    }
  }
  else
  {
    m_CullingCamera = m_Camera;
  }

  if (pView != nullptr)
  {
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", m_pSceneContext->GetRenderSelectionOverlay());
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", m_pSceneContext->GetRenderShapeIcons());
  }
}

ezViewHandle ezSceneViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezVariant sceneContextVariant(m_pSceneContext);
  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorShapeIconsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorGridExtractor", "SceneContext", sceneContextVariant);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCullingCamera(&m_CullingCamera);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  const ezTag& tagHidden = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");

  pView->m_ExcludeTags.Set(tagHidden);
  return pView->GetHandle();
}

void ezSceneViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  ezViewPickingResultMsgToEditor res;
  EZ_SCOPE_EXIT(SendViewMessage(&res));

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView) == false)
    return;

  pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", ezVec2(x, y));

  if (pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickedPosition") == false)
    return;

  ezVariant varPickedPos = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedPosition");
  if (varPickedPos.IsA<ezVec3>() == false)
    return;

  const ezUInt32 uiPickingID = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedID").ConvertTo<ezUInt32>();
  res.m_vPickedNormal = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedNormal").ConvertTo<ezVec3>();
  res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedRayStartPosition").ConvertTo<ezVec3>();
  res.m_vPickedPosition = varPickedPos.ConvertTo<ezVec3>();

  EZ_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

  const ezUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
  const ezUInt32 uiPartIndex = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

  ezArrayPtr<ezWorldRttiConverterContext*> contexts = m_pSceneContext->GetAllContexts();
  for (ezWorldRttiConverterContext* pContext : contexts)
  {
    res.m_ComponentGuid = pContext->m_ComponentPickingMap.GetGuid(uiComponentID);
    if (res.m_ComponentGuid.IsValid() == false)
      continue;

    ezComponentHandle hComponent = pContext->m_ComponentMap.GetHandle(res.m_ComponentGuid);

    ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

    // check whether the component is still valid
    ezComponent* pComponent = nullptr;
    if (pDocumentContext->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
    {
      // if yes, fill out the parent game object guid
      res.m_ObjectGuid = pContext->m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
      res.m_uiPartIndex = uiPartIndex;
    }
    else
    {
      res.m_ComponentGuid = ezUuid();
    }
    break;
  }

  // Always take the other picking ID from the scene itself as gizmos are handled by the window and only the scene itself has one.
  res.m_OtherGuid = m_pSceneContext->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);
}

void ezSceneViewContext::MarqueePickObjects(const ezViewMarqueePickingMsgToEngine* pMsg)
{
  // remote processes do not support picking, just ignore this
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  ezViewMarqueePickingResultMsgToEditor res;
  res.m_uiWhatToDo = pMsg->m_uiWhatToDo;
  res.m_uiActionIdentifier = 0;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos0", ezVec2(pMsg->m_uiPickPosX0, pMsg->m_uiPickPosY0));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos1", ezVec2(pMsg->m_uiPickPosX1, pMsg->m_uiPickPosY1));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueeActionID", pMsg->m_uiActionIdentifier);

    if (pMsg->m_uiWhatToDo == 0xFF)
      return;

    if (!pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "MarqueeActionID") || pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeActionID").ConvertTo<ezUInt32>() != pMsg->m_uiActionIdentifier)
      return;

    res.m_uiActionIdentifier = pMsg->m_uiActionIdentifier;

    ezVariant varMarquee = pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeResult");

    if (varMarquee.IsA<ezVariantArray>())
    {
      ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

      const ezVariantArray resArray = varMarquee.Get<ezVariantArray>();

      for (ezUInt32 i = 0; i < resArray.GetCount(); ++i)
      {
        const ezVariant& singleRes = resArray[i];

        const ezUInt32 uiPickingID = singleRes.ConvertTo<ezUInt32>();
        const ezUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);

        const ezUuid componentGuid = m_pSceneContext->GetActiveContext().m_ComponentPickingMap.GetGuid(uiComponentID);

        if (componentGuid.IsValid())
        {
          ezComponentHandle hComponent = m_pSceneContext->GetActiveContext().m_ComponentMap.GetHandle(componentGuid);

          // check whether the component is still valid
          ezComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuids.PushBack(m_pSceneContext->GetActiveContext().m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle()));
          }
        }
      }
    }
  }

  if (res.m_uiActionIdentifier == 0)
    return;

  SendViewMessage(&res);
}
