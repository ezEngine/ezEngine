#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext)
    : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));

  m_CullingCamera = m_Camera;
}

ezSceneViewContext::~ezSceneViewContext() {}

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
      pView->SetRenderPassProperty("EditorPickingPass.ezGizmoRenderer", "Enabled", pMsg2->m_bEnablePickingSelected);
    }

    if (pMsg2->m_CameraUsageHint == ezCameraUsageHint::EditorView &&
        (pMsg2->m_iCameraMode == ezCameraMode::PerspectiveFixedFovX || pMsg2->m_iCameraMode == ezCameraMode::PerspectiveFixedFovY))
    {
      if (!m_pSceneContext->IsPlayTheGameActive())
      {
        if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
        {
          pSoundInterface->SetListener(-1, pMsg2->m_vPosition, pMsg2->m_vDirForwards, pMsg2->m_vDirUp, ezVec3::ZeroVector());
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

  bool bResult = !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
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

    pView->SetRenderPassProperty("SimplePass.ezGizmoRenderer", "HighlightID", GetDocumentContext()->m_Context.m_uiHighlightID);
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
  bDebugCulling = ezRenderPipeline::s_DebugCulling;
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

  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", m_pSceneContext);
  pView->SetExtractorProperty("EditorShapeIconsExtractor", "SceneContext", m_pSceneContext);
  pView->SetExtractorProperty("EditorGridExtractor", "SceneContext", m_pSceneContext);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCullingCamera(&m_CullingCamera);

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
      res.m_vPickingRayStartPosition =
          pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<ezVec3>();
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

    if (!pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "MarqueeActionID") ||
        pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeActionID").ConvertTo<ezUInt32>() != pMsg->m_uiActionIdentifier)
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

        const ezUuid componentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);

        if (componentGuid.IsValid())
        {
          ezComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(componentGuid);

          // check whether the component is still valid
          ezComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuids.PushBack(GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle()));
          }
        }
      }
    }
  }

  if (res.m_uiActionIdentifier == 0)
    return;

  SendViewMessage(&res);
}
