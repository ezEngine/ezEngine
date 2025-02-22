#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, 1, ezRTTIDefaultAllocator<ezSceneContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab;PropertyAnim"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWorld* ezSceneContext::s_pWorldLinkedWithGameState = nullptr;

void ezSceneContext::ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds)
{
  pObj->UpdateGlobalTransformAndBounds();
  const auto& b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
  {
    ComputeHierarchyBounds(it, bounds);
  }
}

void ezSceneContext::DrawSelectionBounds(const ezViewHandle& hView)
{
  if (!m_bRenderSelectionBoxes)
    return;

  EZ_LOCK(m_pWorld->GetWriteMarker());

  for (const auto& obj : m_Selection)
  {
    ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

    ezGameObject* pObj;
    if (!m_pWorld->TryGetObject(obj, pObj))
      continue;

    ComputeHierarchyBounds(pObj, bounds);

    if (bounds.IsValid())
    {
      ezDebugRenderer::DrawLineBoxCorners(hView, bounds.GetBox(), 0.25f, ezColorScheme::LightUI(ezColorScheme::Yellow));
    }
  }
}

void ezSceneContext::UpdateInvisibleLayerTags()
{
  if (m_bInvisibleLayersDirty)
  {
    m_bInvisibleLayersDirty = false;

    ezMap<ezUuid, ezUInt32> layerGuidToIndex;
    for (ezUInt32 i = 0; i < m_Layers.GetCount(); i++)
    {
      if (m_Layers[i] != nullptr)
      {
        layerGuidToIndex.Insert(m_Layers[i]->GetDocumentGuid(), i);
      }
    }

    ezHybridArray<ezTag, 1> newInvisibleLayerTags;
    newInvisibleLayerTags.Reserve(m_InvisibleLayers.GetCount());
    for (const ezUuid& guid : m_InvisibleLayers)
    {
      ezUInt32 uiLayerID = 0;
      if (layerGuidToIndex.TryGetValue(guid, uiLayerID))
      {
        newInvisibleLayerTags.PushBack(m_Layers[uiLayerID]->GetLayerTag());
      }
      else if (guid == GetDocumentGuid())
      {
        newInvisibleLayerTags.PushBack(m_LayerTag);
      }
    }

    for (ezEngineProcessViewContext* pView : m_ViewContexts)
    {
      if (pView)
      {
        static_cast<ezSceneViewContext*>(pView)->SetInvisibleLayerTags(m_InvisibleLayerTags.GetArrayPtr(), newInvisibleLayerTags.GetArrayPtr());
      }
    }
    m_InvisibleLayerTags.Swap(newInvisibleLayerTags);
  }
}

ezSceneContext::ezSceneContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
  m_bRenderSelectionOverlay = true;
  m_bRenderSelectionBoxes = true;
  m_bRenderShapeIcons = true;
  m_fGridDensity = 0;
  m_GridTransform.SetIdentity();
  m_pWorld = nullptr;

  ezResourceManager::GetManagerEvents().AddEventHandler(ezMakeDelegate(&ezSceneContext::OnResourceManagerEvent, this));
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezSceneContext::GameApplicationEventHandler, this));
}

ezSceneContext::~ezSceneContext()
{
  ezResourceManager::GetManagerEvents().RemoveEventHandler(ezMakeDelegate(&ezSceneContext::OnResourceManagerEvent, this));
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneContext::GameApplicationEventHandler, this));
}

void ezSceneContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezWorldSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages
    HandleWorldSettingsMsg(static_cast<const ezWorldSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezSimulationSettingsMsgToEngine>())
  {
    HandleSimulationSettingsMsg(static_cast<const ezSimulationSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGridSettingsMsgToEngine>())
  {
    HandleGridSettingsMsg(static_cast<const ezGridSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectsForDebugVisMsgToEngine>())
  {
    HandleObjectsForDebugVisMsg(static_cast<const ezObjectsForDebugVisMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGameModeMsgToEngine>())
  {
    HandleGameModeMsg(static_cast<const ezGameModeMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const ezObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezExposedDocumentObjectPropertiesMsgToEngine>())
  {
    HandleExposedPropertiesMsg(static_cast<const ezExposedDocumentObjectPropertiesMsgToEngine*>(pMsg));
    return;
  }

  if (const ezExportSceneGeometryMsgToEngine* msg = ezDynamicCast<const ezExportSceneGeometryMsgToEngine*>(pMsg))
  {
    HandleSceneGeometryMsg(msg);
    return;
  }

  if (const ezPullObjectStateMsgToEngine* msg = ezDynamicCast<const ezPullObjectStateMsgToEngine*>(pMsg))
  {
    HandlePullObjectStateMsg(msg);
    return;
  }

  if (pMsg->IsInstanceOf<ezViewRedrawMsgToEngine>())
  {
    HandleViewRedrawMsg(static_cast<const ezViewRedrawMsgToEngine*>(pMsg));
    // fall through
  }

  if (pMsg->IsInstanceOf<ezActiveLayerChangedMsgToEngine>())
  {
    HandleActiveLayerChangedMsg(static_cast<const ezActiveLayerChangedMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<ezObjectTagMsgToEngine>())
  {
    HandleTagMsgToEngineMsg(static_cast<const ezObjectTagMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<ezLayerVisibilityChangedMsgToEngine>())
  {
    HandleLayerVisibilityChangedMsgToEngineMsg(static_cast<const ezLayerVisibilityChangedMsgToEngine*>(pMsg));
    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);

  if (pMsg->IsInstanceOf<ezEntityMsgToEngine>())
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());
    AddLayerIndexTag(*static_cast<const ezEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void ezSceneContext::HandleViewRedrawMsg(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_bUpdateAllLocalBounds)
  {
    m_bUpdateAllLocalBounds = false;

    EZ_LOCK(m_pWorld->GetWriteMarker());

    for (auto it = m_pWorld->GetObjects(); it.IsValid(); ++it)
    {
      it->UpdateLocalBounds();
    }
  }

  auto pDocView = GetViewContext(pMsg->m_uiViewID);
  if (pDocView)
    DrawSelectionBounds(pDocView->GetViewHandle());

  AnswerObjectStatePullRequest(pMsg);
  UpdateInvisibleLayerTags();
}

void ezSceneContext::AnswerObjectStatePullRequest(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pWorld->GetWorldSimulationEnabled() || m_PushObjectStateMsg.m_ObjectStates.IsEmpty())
    return;

  EZ_LOCK(m_pWorld->GetReadMarker());

  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    ezWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      return;

    // if the handle map is currently empty, the scene has not yet been sent over
    // return and try again later
    if (pContext->m_GameObjectMap.GetHandleToGuidMap().IsEmpty())
      return;
  }

  // now we need to adjust the transforms for all objects that were not directly pulled
  // ie. nodes inside instantiated prefabs
  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    // ignore the ones that we accessed directly, their transform is correct already
    if (!state.m_bAdjustFromPrefabRootChild)
      continue;

    ezWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      continue;

    const auto& objectMapper = pContext->m_GameObjectMap;

    ezGameObjectHandle hObject = objectMapper.GetHandle(state.m_ObjectGuid);

    // if this object does not exist anymore, this is not considered a problem (user may have deleted it)
    ezGameObject* pObject;
    if (!m_pWorld->TryGetObject(hObject, pObject))
      continue;

    // we expect the object to have a child, if none is there yet, we assume the prefab
    // instantiation has not happened yet
    // stop the whole process and try again later
    if (pObject->GetChildCount() == 0)
      return;

    const ezGameObject* pChild = pObject->GetChildren();

    const ezVec3 localPos = pChild->GetLocalPosition();
    const ezQuat localRot = pChild->GetLocalRotation();

    // now adjust the position
    state.m_vPosition -= state.m_qRotation * localRot.GetInverse() * localPos;
    state.m_qRotation = state.m_qRotation * localRot.GetInverse();
  }

  // send a return message with the result
  m_PushObjectStateMsg.m_DocumentGuid = pMsg->m_DocumentGuid;
  SendProcessMessage(&m_PushObjectStateMsg);

  m_PushObjectStateMsg.m_ObjectStates.Clear();
}

void ezSceneContext::HandleActiveLayerChangedMsg(const ezActiveLayerChangedMsgToEngine* pMsg)
{
  m_ActiveLayer = pMsg->m_ActiveLayer;
}

void ezSceneContext::HandleTagMsgToEngineMsg(const ezObjectTagMsgToEngine* pMsg)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezGameObjectHandle hObject = GetActiveContext().m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(pMsg->m_sTag);

  ezGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (pMsg->m_bApplyOnAllChildren)
    {
      if (pMsg->m_bSetTag)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (pMsg->m_bSetTag)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void ezSceneContext::HandleLayerVisibilityChangedMsgToEngineMsg(const ezLayerVisibilityChangedMsgToEngine* pMsg)
{
  m_InvisibleLayers = pMsg->m_HiddenLayers;
  m_bInvisibleLayersDirty = true;
}

void ezSceneContext::HandleGridSettingsMsg(const ezGridSettingsMsgToEngine* pMsg)
{
  m_fGridDensity = pMsg->m_fGridDensity;
  if (m_fGridDensity != 0.0f)
  {
    m_GridTransform.m_vPosition = pMsg->m_vGridCenter;

    if (pMsg->m_vGridTangent1.IsZero())
    {
      m_GridTransform.m_vScale.SetZero();
    }
    else
    {
      m_GridTransform.m_vScale.Set(1.0f);

      ezMat3 mRot;
      mRot.SetColumn(0, pMsg->m_vGridTangent1);
      mRot.SetColumn(1, pMsg->m_vGridTangent2);
      mRot.SetColumn(2, pMsg->m_vGridTangent1.CrossRH(pMsg->m_vGridTangent2));
      m_GridTransform.m_qRotation = ezQuat::MakeFromMat3(mRot);
    }
  }
}

void ezSceneContext::HandleSimulationSettingsMsg(const ezSimulationSettingsMsgToEngine* pMsg)
{
  const bool bSimulate = pMsg->m_bSimulateWorld;
  ezGameStateBase* pState = GetGameState();
  m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);

  if (pState == nullptr && bSimulate != m_pWorld->GetWorldSimulationEnabled())
  {
    m_pWorld->SetWorldSimulationEnabled(bSimulate);

    if (bSimulate)
      OnSimulationEnabled();
    else
      OnSimulationDisabled();
  }
}

void ezSceneContext::HandleWorldSettingsMsg(const ezWorldSettingsMsgToEngine* pMsg)
{
  m_bRenderSelectionOverlay = pMsg->m_bRenderOverlay;
  m_bRenderShapeIcons = pMsg->m_bRenderShapeIcons;
  m_bRenderSelectionBoxes = pMsg->m_bRenderSelectionBoxes;

  if (pMsg->m_bAddAmbientLight)
    AddAmbientLight(true, false);
  else
    RemoveAmbientLight();
}

void ezSceneContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    for (const auto& obj : m_Selection)
    {
      ezGameObject* pObj;
      if (!m_pWorld->TryGetObject(obj, pObj))
        continue;

      ComputeHierarchyBounds(pObj, bounds);
    }

    // if there are no valid bounds, at all, use dummy bounds for each object
    if (!bounds.IsValid())
    {
      for (const auto& obj : m_Selection)
      {
        ezGameObject* pObj;
        if (!m_pWorld->TryGetObject(obj, pObj))
          continue;

        bounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromCenterExtents(pObj->GetGlobalPosition(), ezVec3(0.0f), 0.0f));
      }
    }
  }

  // EZ_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

  if (!bounds.IsValid() || bounds.IsNaN())
  {
    ezLog::Error("Selection has no valid bounding box");
    return;
  }

  const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

  ezQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void ezSceneContext::OnSimulationEnabled()
{
  ezLog::Info("World Simulation enabled");

  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  ezResourceManager::ReloadAllResources(false);

  ezGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(true);
  }
}

void ezSceneContext::OnSimulationDisabled()
{
  ezLog::Info("World Simulation disabled");

  ezResourceManager::ResetAllResources();

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

ezGameStateBase* ezSceneContext::GetGameState() const
{
  if (s_pWorldLinkedWithGameState == m_pWorld)
  {
    return ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState();
  }

  return nullptr;
}

ezUInt32 ezSceneContext::RegisterLayer(ezLayerContext* pLayer)
{
  m_bInvisibleLayersDirty = true;
  m_Contexts.PushBack(&pLayer->m_Context);
  for (ezUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == nullptr)
    {
      m_Layers[i] = pLayer;
      return i;
    }
  }

  m_Layers.PushBack(pLayer);
  return m_Layers.GetCount() - 1;
}

void ezSceneContext::UnregisterLayer(ezLayerContext* pLayer)
{
  m_Contexts.RemoveAndSwap(&pLayer->m_Context);
  for (ezUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == pLayer)
    {
      m_Layers[i] = nullptr;
    }
  }

  while (!m_Layers.IsEmpty() && m_Layers.PeekBack() == nullptr)
    m_Layers.PopBack();
}

void ezSceneContext::AddLayerIndexTag(const ezEntityMsgToEngine& msg, ezWorldRttiConverterContext& ref_context, const ezTag& layerTag)
{
  if (msg.m_change.m_Change.m_Operation == ezObjectChangeType::NodeAdded)
  {
    if ((msg.m_change.m_Change.m_sProperty == "Children" || msg.m_change.m_Change.m_sProperty.IsEmpty()) && msg.m_change.m_Change.m_Value.IsA<ezUuid>())
    {
      const ezUuid& object = msg.m_change.m_Change.m_Value.Get<ezUuid>();
      ezRttiConverterObject target = ref_context.GetObjectByGUID(object);
      if (target.m_pType == ezGetStaticRTTI<ezGameObject>() && target.m_pObject != nullptr)
      {
        // We do postpone tagging until after the first frame so that prefab references are instantiated and affected as well.
        ezGameObject* pObject = static_cast<ezGameObject*>(target.m_pObject);
        m_ObjectsToTag.PushBack({pObject->GetHandle(), layerTag});
      }
    }
  }
}

const ezArrayPtr<const ezTag> ezSceneContext::GetInvisibleLayerTags() const
{
  return m_InvisibleLayerTags.GetArrayPtr();
}

void ezSceneContext::OnInitialize()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());
  if (!m_ActiveLayer.IsValid())
    m_ActiveLayer = m_DocumentGuid;
  m_Contexts.PushBack(&m_Context);

  m_LayerTag = ezTagRegistry::GetGlobalRegistry().RegisterTag("Layer_Scene");

  ezShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void ezSceneContext::OnDeinitialize()
{
  m_Selection.Clear();
  m_SelectionWithChildren.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_hSkyLight.Invalidate();
  m_hDirectionalLight.Invalidate();
  m_LayerTag = ezTag();
  for (ezLayerContext* pLayer : m_Layers)
  {
    if (pLayer != nullptr)
      pLayer->SceneDeinitialized();
  }
}

ezEngineProcessViewContext* ezSceneContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezSceneViewContext, this);
}

void ezSceneContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezSceneContext::HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  ezStringBuilder sSel = pMsg->m_sSelection;
  ezStringBuilder sGuid;

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = GetActiveContext().m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      ezGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void ezSceneContext::OnPlayTheGameModeStarted(ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  if (ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState() != nullptr)
  {
    ezLog::Warning("A Play-the-Game instance is already running, cannot launch a second in parallel.");
    return;
  }

  ezLog::Info("Starting Play-the-Game mode");

  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  ezResourceManager::ReloadAllResources(false);

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  ezGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  s_pWorldLinkedWithGameState = m_pWorld;
  ezGameApplicationBase::GetGameApplicationBaseInstance()->ActivateGameState(m_pWorld, sStartPosition, startPositionOffset);

  ezGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG = true;

  SendProcessMessage(&msgRet);

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

void ezSceneContext::OnResourceManagerEvent(const ezResourceManagerEvent& e)
{
  if (e.m_Type == ezResourceManagerEvent::Type::ReloadAllResources)
  {
    // when resources get reloaded, make sure to update all object bounds
    // this is to prevent culling errors after meshes got transformed etc.
    m_bUpdateAllLocalBounds = true;
  }
}

void ezSceneContext::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::AfterUpdatePlugins && !m_ObjectsToTag.IsEmpty())
  {
    // At this point the world was ticked once and prefab instances are instantiated and will be affected by SetTagRecursive.
    EZ_LOCK(m_pWorld->GetWriteMarker());
    for (const TagGameObject& tagObject : m_ObjectsToTag)
    {
      ezGameObject* pObject = nullptr;
      if (m_pWorld->TryGetObject(tagObject.m_hObject, pObject))
      {
        SetTagRecursive(pObject, tagObject.m_Tag);
      }
    }
    m_ObjectsToTag.Clear();
  }
}

void ezSceneContext::HandleObjectsForDebugVisMsg(const ezObjectsForDebugVisMsgToEngine* pMsg)
{
  EZ_LOCK(GetWorld()->GetWriteMarker());

  const ezArrayPtr<const ezUuid> guids(reinterpret_cast<const ezUuid*>(pMsg->m_Objects.GetData()), pMsg->m_Objects.GetCount() / sizeof(ezUuid));

  for (auto guid : guids)
  {
    auto hComp = GetActiveContext().m_ComponentMap.GetHandle(guid);

    if (hComp.IsInvalidated())
      continue;

    ezEventMessageHandlerComponent* pComp = nullptr;
    if (!m_pWorld->TryGetComponent(hComp, pComp))
      continue;

    pComp->SetDebugOutput(true);
  }
}

void ezSceneContext::HandleGameModeMsg(const ezGameModeMsgToEngine* pMsg)
{
  ezGameStateBase* pState = GetGameState();

  if (pMsg->m_bEnablePTG)
  {
    if (pState != nullptr)
    {
      ezLog::Error("Cannot start Play-the-Game, there is already a game state active for this world");
      return;
    }

    if (pMsg->m_bUseStartPosition)
    {
      ezQuat qRot = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), pMsg->m_vStartDirection);

      ezTransform tStart(pMsg->m_vStartPosition, qRot);

      OnPlayTheGameModeStarted("GlobalOverride", tStart);
    }
    else
    {
      OnPlayTheGameModeStarted({}, ezTransform::MakeIdentity());
    }
  }
  else
  {
    if (pState == nullptr)
      return;

    ezLog::Info("Attempting to stop Play-the-Game mode");
    pState->RequestQuit();
  }
}

void ezSceneContext::InsertSelectedChildren(const ezGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}

ezStatus ezSceneContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  if (!m_Context.m_UnknownTypes.IsEmpty())
  {
    ezStringBuilder s;

    s.Append("Scene / prefab export failed: ");

    for (const ezString& sType : m_Context.m_UnknownTypes)
    {
      s.AppendFormat("'{}' is unknown. ", sType);
    }

    return ezStatus(s.GetView());
  }

  // make sure the world has been updated at least once, otherwise components aren't initialized
  // and messages for geometry extraction won't be delivered
  // this is necessary for the scene export modifiers to work
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());
    m_pWorld->SetWorldSimulationEnabled(false);
    m_pWorld->Update();
  }

  // #TODO layers
  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), true);

  ezDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // export
  {
    // File Header
    {
      ezAssetFileHeader header;
      header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
      header.Write(file).IgnoreResult();

      const char* szSceneTag = "[ezBinaryScene]";
      file.WriteBytes(szSceneTag, sizeof(char) * 16).IgnoreResult();
    }

    const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    const ezTag& tagNoExport = ezTagRegistry::GetGlobalRegistry().RegisterTag("Exclude From Export");

    ezTagSet tags;
    tags.Set(tagEditor);
    tags.Set(tagNoExport);

    ezWorldWriter ww;
    ww.WriteWorld(file, *m_pWorld, &tags);

    ExportExposedParameters(ww, file);
  }

  // do the actual file writing
  if (file.Close().Failed())
    return ezStatus(ezFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return ezStatus(EZ_SUCCESS);
}

void ezSceneContext::ExportExposedParameters(const ezWorldWriter& ww, ezDeferredFileWriter& file) const
{
  ezHybridArray<ezExposedPrefabParameterDesc, 16> exposedParams;

  for (const auto& esp : m_ExposedSceneProperties)
  {
    ezGameObject* pTargetObject = nullptr;
    const ezRTTI* pComponenType = nullptr;

    ezRttiConverterObject obj = m_Context.GetObjectByGUID(esp.m_Object);

    if (obj.m_pType == nullptr)
      continue;

    if (obj.m_pType->IsDerivedFrom<ezGameObject>())
    {
      pTargetObject = reinterpret_cast<ezGameObject*>(obj.m_pObject);
    }
    else if (obj.m_pType->IsDerivedFrom<ezComponent>())
    {
      ezComponent* pComponent = reinterpret_cast<ezComponent*>(obj.m_pObject);

      pTargetObject = pComponent->GetOwner();
      pComponenType = obj.m_pType;
    }

    if (pTargetObject == nullptr)
      continue;

    ezInt32 iFoundObjRoot = -1;
    ezInt32 iFoundObjChild = -1;

    // search for the target object in the exported objects
    {
      const auto& objects = ww.GetAllWrittenRootObjects();
      for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
      {
        if (objects[i] == pTargetObject)
        {
          iFoundObjRoot = i;
          break;
        }
      }

      if (iFoundObjRoot < 0)
      {
        const auto& objects = ww.GetAllWrittenChildObjects();
        for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
        {
          if (objects[i] == pTargetObject)
          {
            iFoundObjChild = i;
            break;
          }
        }
      }
    }

    // if exposed object not found, ignore parameter
    if (iFoundObjRoot < 0 && iFoundObjChild < 0)
      continue;

    // store the exposed parameter information
    ezExposedPrefabParameterDesc& paramdesc = exposedParams.ExpandAndGetRef();
    paramdesc.m_sExposeName.Assign(esp.m_sName.GetData());
    paramdesc.m_uiWorldReaderChildObject = (iFoundObjChild >= 0) ? 1 : 0;
    paramdesc.m_uiWorldReaderObjectIndex = (iFoundObjChild >= 0) ? iFoundObjChild : iFoundObjRoot;
    paramdesc.m_sComponentType.Clear();

    if (pComponenType)
    {
      paramdesc.m_sComponentType.Assign(pComponenType->GetTypeName());
    }

    paramdesc.m_sProperty.Assign(esp.m_sPropertyPath.GetData());
  }

  exposedParams.Sort([](const ezExposedPrefabParameterDesc& lhs, const ezExposedPrefabParameterDesc& rhs) -> bool
    { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });

  file << exposedParams.GetCount();

  for (const auto& ep : exposedParams)
  {
    ep.Save(file);
  }
}

void ezSceneContext::OnThumbnailViewContextCreated()
{
  // make sure there is ambient light in the thumbnails
  // TODO: should check whether this is a prefab (info currently not available in ezSceneContext)
  RemoveAmbientLight();
  AddAmbientLight(false, true);
}

void ezSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}

void ezSceneContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();

  if (ezGameStateBase* pState = GetGameState())
  {
    // If we have a running game state we always want to render it (e.g. play the game).
    pState->AddMainViewsToRender();

    if (pState->WasQuitRequested())
    {
      ezGameApplicationBase::GetGameApplicationBaseInstance()->DeactivateGameState();
      s_pWorldLinkedWithGameState = nullptr;

      ezGameModeMsgToEditor msgToEd;
      msgToEd.m_DocumentGuid = GetDocumentGuid();
      msgToEd.m_bRunningPTG = false;

      SendProcessMessage(&msgToEd);
    }
  }
}

ezGameObjectHandle ezSceneContext::ResolveStringToGameObjectHandle(const void* pString, ezComponentHandle hThis, ezStringView sProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pString);

  if (hThis.IsInvalidated() && sProperty.IsEmpty())
  {
    // This code path is used by ezPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!ezConversionUtils::IsStringUuid(szTargetGuid))
      return ezGameObjectHandle();

    // convert string to GUID and check if references a known object
    return m_Context.m_GameObjectMap.GetHandle(ezConversionUtils::ConvertStringToUuid(szTargetGuid));
  }

  // Test if the component is a direct part of this scene or one of its layers.
  if (m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
  {
    return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
  }
  for (const ezLayerContext* pLayer : m_Layers)
  {
    if (pLayer)
    {
      if (pLayer->m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
      {
        return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
      }
    }
  }

  // Component not found - it is probably an engine prefab instance part.
  // Walk up the hierarchy and find a game object that belongs to the scene or layer.
  ezComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent<ezComponent>(hThis, pComponent))
    return {};

  const ezGameObject* pParent = pComponent->GetOwner();
  while (pParent)
  {
    if (m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
    {
      return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
    }
    for (const ezLayerContext* pLayer : m_Layers)
    {
      if (pLayer)
      {
        if (pLayer->m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
        {
          return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
        }
      }
    }
    pParent = pParent->GetParent();
  }

  ezLog::Error("Game object reference could not be resolved. Component source was not found.");
  return ezGameObjectHandle();
}

bool ezSceneContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  const ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezSceneViewContext* pMaterialViewContext = static_cast<ezSceneViewContext*>(pThumbnailViewContext);
  const bool result = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void ezSceneContext::AddAmbientLight(bool bSetEditorTag, bool bForce)
{
  if (!m_hSkyLight.IsInvalidated() || !m_hDirectionalLight.IsInvalidated())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  // delay adding ambient light until the scene isn't empty, to prevent adding two skylights
  if (!bForce && GetWorld()->GetObjectCount() == 0)
    return;

  ezSkyLightComponentManager* pSkyMan = GetWorld()->GetComponentManager<ezSkyLightComponentManager>();
  if (pSkyMan == nullptr || pSkyMan->GetSingletonComponent() == nullptr)
  {
    // only create a skylight, if there is none yet

    ezGameObjectDesc obj;
    obj.m_sName.Assign("Sky Light");

    if (bSetEditorTag)
    {
      const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    ezGameObject* pObj;
    m_hSkyLight = GetWorld()->CreateObject(obj, pObj);


    ezSkyLightComponent* pSkyLight = nullptr;
    ezSkyLightComponent::CreateComponent(pObj, pSkyLight);
    pSkyLight->SetCubeMapFile("{ 0b202e08-a64f-465d-b38e-15b81d161822 }");
    pSkyLight->SetReflectionProbeMode(ezReflectionProbeMode::Static);
  }

  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("Ambient Light");

    obj.m_LocalRotation = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(-14.510815f), ezAngle::MakeFromDegree(43.07951f), ezAngle::MakeFromDegree(93.223808f));

    if (bSetEditorTag)
    {
      const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    ezGameObject* pLight;
    m_hDirectionalLight = GetWorld()->CreateObject(obj, pLight);

    ezDirectionalLightComponent* pDirLight = nullptr;
    ezDirectionalLightComponent::CreateComponent(pLight, pDirLight);
    pDirLight->SetIntensity(10.0f);
  }
}

void ezSceneContext::RemoveAmbientLight()
{
  EZ_LOCK(GetWorld()->GetWriteMarker());

  if (!m_hSkyLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hSkyLight);
    m_hSkyLight.Invalidate();
  }

  if (!m_hDirectionalLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hDirectionalLight);
    m_hDirectionalLight.Invalidate();
  }
}

const ezEngineProcessDocumentContext* ezSceneContext::GetActiveDocumentContext() const
{
  if (m_ActiveLayer == GetDocumentGuid())
  {
    return this;
  }

  for (const ezLayerContext* pLayer : m_Layers)
  {
    if (pLayer && m_ActiveLayer == pLayer->GetDocumentGuid())
    {
      return pLayer;
    }
  }

  EZ_REPORT_FAILURE("Active layer does not exist.");
  return this;
}

ezEngineProcessDocumentContext* ezSceneContext::GetActiveDocumentContext()
{
  return const_cast<ezEngineProcessDocumentContext*>(const_cast<const ezSceneContext*>(this)->GetActiveDocumentContext());
}

const ezWorldRttiConverterContext& ezSceneContext::GetActiveContext() const
{
  return GetActiveDocumentContext()->m_Context;
}

ezWorldRttiConverterContext& ezSceneContext::GetActiveContext()
{
  return const_cast<ezWorldRttiConverterContext&>(const_cast<const ezSceneContext*>(this)->GetActiveContext());
}

ezWorldRttiConverterContext* ezSceneContext::GetContextForLayer(const ezUuid& layerGuid)
{
  if (layerGuid == GetDocumentGuid())
    return &m_Context;

  for (ezLayerContext* pLayer : m_Layers)
  {
    if (pLayer && layerGuid == pLayer->GetDocumentGuid())
    {
      return &pLayer->m_Context;
    }
  }
  return nullptr;
}

ezArrayPtr<ezWorldRttiConverterContext*> ezSceneContext::GetAllContexts()
{
  return m_Contexts;
}

void ezSceneContext::HandleExposedPropertiesMsg(const ezExposedDocumentObjectPropertiesMsgToEngine* pMsg)
{
  m_ExposedSceneProperties = pMsg->m_Properties;
}

void ezSceneContext::HandleSceneGeometryMsg(const ezExportSceneGeometryMsgToEngine* pMsg)
{
  ezWorldGeoExtractionUtil::MeshObjectList objects;

  ezTagSet excludeTags;
  excludeTags.SetByName("Editor");

  if (pMsg->m_bSelectionOnly)
    ezWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<ezWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), m_SelectionWithChildren);
  else
    ezWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<ezWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), &excludeTags);

  ezWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(pMsg->m_sOutputFile, objects, pMsg->m_Transform);
}

void ezSceneContext::HandlePullObjectStateMsg(const ezPullObjectStateMsgToEngine* pMsg)
{
  if (!m_pWorld->GetWorldSimulationEnabled())
    return;

  const ezWorld* pWorld = GetWorld();
  EZ_LOCK(pWorld->GetReadMarker());

  const auto& objectMapper = GetActiveContext().m_GameObjectMap;

  m_PushObjectStateMsg.m_ObjectStates.Reserve(m_PushObjectStateMsg.m_ObjectStates.GetCount() + m_SelectionWithChildren.GetCount());

  for (ezGameObjectHandle hObject : m_SelectionWithChildren)
  {
    const ezGameObject* pObject = nullptr;
    if (!pWorld->TryGetObject(hObject, pObject))
      continue;

    ezUuid objectGuid = objectMapper.GetGuid(hObject);
    bool bAdjust = false;

    if (!objectGuid.IsValid())
    {
      // this must be an object created on the runtime side, try to match it to some editor object
      // we only try the direct parent, more steps than that are not allowed

      const ezGameObject* pParentObject = pObject->GetParent();
      if (pParentObject == nullptr)
        continue;

      // if the parent has more than one child, remapping the position from the child to the parent is not possible, so skip those
      if (pParentObject->GetChildCount() > 1)
        continue;

      auto parentGuid = objectMapper.GetGuid(pParentObject->GetHandle());

      if (!parentGuid.IsValid())
        continue;

      objectGuid = parentGuid;
      bAdjust = true;

      for (ezUInt32 i = 0; i < m_PushObjectStateMsg.m_ObjectStates.GetCount(); ++i)
      {
        if (m_PushObjectStateMsg.m_ObjectStates[i].m_ObjectGuid == objectGuid)
        {
          m_PushObjectStateMsg.m_ObjectStates.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    {
      auto& state = m_PushObjectStateMsg.m_ObjectStates.ExpandAndGetRef();

      state.m_LayerGuid = m_ActiveLayer;
      state.m_ObjectGuid = objectGuid;
      state.m_bAdjustFromPrefabRootChild = bAdjust;
      state.m_vPosition = pObject->GetGlobalPosition();
      state.m_qRotation = pObject->GetGlobalRotation();

      ezMsgRetrieveBoneState msg;
      pObject->SendMessage(msg);

      state.m_BoneTransforms = msg.m_BoneTransforms;
    }
  }

  // the return message is sent after the simulation has stopped
}
