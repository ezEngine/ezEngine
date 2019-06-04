#include <EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <SharedPluginScene/Common/Messages.h>


#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, 1, ezRTTIDefaultAllocator<ezSceneContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab;PropertyAnim Asset"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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
    ezBoundingBoxSphere bounds;
    bounds.SetInvalid();

    ezGameObject* pObj;
    if (!m_pWorld->TryGetObject(obj, pObj))
      continue;

    ComputeHierarchyBounds(pObj, bounds);

    if (bounds.IsValid())
    {
      ezDebugRenderer::DrawLineBoxCorners(hView, bounds.GetBox(), 0.25f, ezColor::Yellow);
    }
  }
}

ezSceneContext::ezSceneContext()
{
  m_bRenderSelectionOverlay = true;
  m_bRenderSelectionBoxes = true;
  m_bRenderShapeIcons = true;
  m_fGridDensity = 0;
  m_GridTransform.SetIdentity();
  m_pWorld = nullptr;

  ezVisualScriptComponent::s_ActivityEvents.AddEventHandler(ezMakeDelegate(&ezSceneContext::OnVisualScriptActivity, this));
  ezResourceManager::s_ManagerEvents.AddEventHandler(ezMakeDelegate(&ezSceneContext::OnResourceManagerEvent, this));
}

ezSceneContext::~ezSceneContext()
{
  ezVisualScriptComponent::s_ActivityEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneContext::OnVisualScriptActivity, this));
  ezResourceManager::s_ManagerEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneContext::OnResourceManagerEvent, this));
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

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGlobalSettingsMsgToEngine>())
  {
    HandleGlobalSettingsMsg(static_cast<const ezGlobalSettingsMsgToEngine*>(pMsg));
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
    {
      auto op = EZ_DEFAULT_NEW(ezLongOperationLocal_Dummy);
      op->m_Duration = ezTime::Seconds(2.0);
      ezLongOperationManager::GetSingleton()->AddLongOperation(std::move(op), this->GetDocumentGuid());
    }

    HandlePullObjectStateMsg(msg);
    return;
  }

  if (pMsg->IsInstanceOf<ezViewRedrawMsgToEngine>())
  {
    HandleViewRedrawMsg(static_cast<const ezViewRedrawMsgToEngine*>(pMsg));
    // fall through
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
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
}

void ezSceneContext::AnswerObjectStatePullRequest(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pWorld->GetWorldSimulationEnabled() || m_PushObjectStateMsg.m_ObjectStates.IsEmpty())
    return;

  EZ_LOCK(m_pWorld->GetReadMarker());

  const auto& objectMapper = GetDocumentContext(GetDocumentGuid())->m_Context.m_GameObjectMap;

  // if the handle map is currently empty, the scene has not yet been sent over
  // return and try again later
  if (objectMapper.GetHandleToGuidMap().IsEmpty())
    return;

  // now we need to adjust the transforms for all objects that were not directly pulled
  // ie. nodes inside instantiated prefabs
  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    // ignore the ones that we accessed directly, their transform is correct already
    if (!state.m_bAdjustFromPrefabRootChild)
      continue;

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
    state.m_vPosition -= state.m_qRotation * -localRot * localPos;
    state.m_qRotation = state.m_qRotation * -localRot;
  }

  // send a return message with the result
  m_PushObjectStateMsg.m_DocumentGuid = pMsg->m_DocumentGuid;
  SendProcessMessage(&m_PushObjectStateMsg);

  m_PushObjectStateMsg.m_ObjectStates.Clear();
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
      m_GridTransform.m_qRotation.SetFromMat3(mRot);
    }
  }
}

void ezSceneContext::HandleGlobalSettingsMsg(const ezGlobalSettingsMsgToEngine* pMsg)
{
  ezGizmoRenderer::s_fGizmoScale = pMsg->m_fGizmoScale;
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
    AddAmbientLight(true);
  else
    RemoveAmbientLight();
}

void ezSceneContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

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

        bounds.ExpandToInclude(ezBoundingBoxSphere(pObj->GetGlobalPosition(), ezVec3(0.0f), 0.0f));
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
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void ezSceneContext::OnSimulationEnabled()
{
  ezLog::Info("World Simulation enabled");

  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid());

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
  return ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(m_pWorld.Borrow());
}

void ezSceneContext::OnInitialize()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());
}

void ezSceneContext::OnDeinitialize()
{
  m_Selection.Clear();
  m_SelectionWithChildren.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_hAmbientLight[0].Invalidate();
  m_hAmbientLight[1].Invalidate();
  m_hAmbientLight[2].Invalidate();
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

  auto pWorld = m_pWorld.Borrow();
  EZ_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = m_Context.m_GameObjectMap.GetHandle(guid);

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

void ezSceneContext::OnPlayTheGameModeStarted(const ezTransform* pStartPosition)
{
  if (ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState() != nullptr)
  {
    ezLog::Warning("A Play-the-Game instance is already running, cannot launch a second in parallel.");
    return;
  }

  ezLog::Info("Starting Play-the-Game mode");

  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid());

  ezResourceManager::ReloadAllResources(false);

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  ezGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  ezGameApplicationBase::GetGameApplicationBaseInstance()->ActivateGameState(m_pWorld.Borrow(), pStartPosition);

  ezGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG = true;

  SendProcessMessage(&msgRet);

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}


void ezSceneContext::OnVisualScriptActivity(const ezVisualScriptComponentActivityEvent& e)
{
  // component handles are not unique across different worlds, in fact it is very likely that different worlds contain identical handles
  // therefore we first need to filter out, whether the component comes from the same world, as this context operates on
  if (e.m_pComponent->GetWorld() != GetWorld())
    return;

  EZ_ASSERT_DEV(e.m_pComponent->GetDebugOutput(), "This component should not send debug data.");

  const ezUuid guid = m_Context.m_ComponentMap.GetGuid(e.m_pComponent->GetHandle());

  if (!guid.IsValid())
    return;

  ezVisualScriptActivityMsgToEditor msg;
  msg.m_DocumentGuid = GetDocumentGuid();
  msg.m_ComponentGuid = guid;

  ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_Activity);
  ezMemoryStreamWriter writer(&storage);

  writer << e.m_pActivity->m_ActiveExecutionConnections.GetCount();
  writer << e.m_pActivity->m_ActiveDataConnections.GetCount();

  for (const auto& con : e.m_pActivity->m_ActiveExecutionConnections)
  {
    writer << con;
  }

  for (const auto& con : e.m_pActivity->m_ActiveDataConnections)
  {
    writer << con;
  }


  SendProcessMessage(&msg);
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

void ezSceneContext::HandleObjectsForDebugVisMsg(const ezObjectsForDebugVisMsgToEngine* pMsg)
{
  EZ_LOCK(GetWorld()->GetWriteMarker());

  const ezArrayPtr<const ezUuid> guids(reinterpret_cast<const ezUuid*>(pMsg->m_Objects.GetData()),
                                       pMsg->m_Objects.GetCount() / sizeof(ezUuid));

  for (auto guid : guids)
  {
    auto hComp = m_Context.m_ComponentMap.GetHandle(guid);

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
      ezQuat qRot;
      qRot.SetShortestRotation(ezVec3(1, 0, 0), pMsg->m_vStartDirection);

      ezTransform tStart(pMsg->m_vStartPosition, qRot);

      OnPlayTheGameModeStarted(&tStart);
    }
    else
    {
      OnPlayTheGameModeStarted(nullptr);
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

bool ezSceneContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid());

  ezDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // export
  {
    // File Header
    {
      ezAssetFileHeader header;
      header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
      header.Write(file);

      const char* szSceneTag = "[ezBinaryScene]";
      file.WriteBytes(szSceneTag, sizeof(char) * 16);
    }

    const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    const ezTag& tagEditorPrefabInstance = ezTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

    ezTagSet tags;
    tags.Set(tagEditor);
    tags.Set(tagEditorPrefabInstance);

    ezWorldWriter ww;
    ww.WriteWorld(file, *m_pWorld, &tags);

    ExportExposedParameters(ww, file);
  }

  // do the actual file writing
  return file.Close().Succeeded();
}

void ezSceneContext::ExportExposedParameters(const ezWorldWriter& ww, ezDeferredFileWriter& file) const
{
  ezHybridArray<ezExposedPrefabParameterDesc, 16> exposedParams;

  for (const auto& esp : m_ExposedSceneProperties)
  {
    ezGameObject* pTargetObject = nullptr;
    const ezRTTI* pComponenType = nullptr;

    ezRttiConverterObject obj = m_Context.GetObjectByGUID(esp.m_Object);

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
    paramdesc.m_uiComponentTypeHash = pComponenType ? pComponenType->GetTypeNameHash() : 0;
    paramdesc.m_sProperty.Assign(esp.m_sPropertyPath.GetData());
  }

  exposedParams.Sort([](const ezExposedPrefabParameterDesc& lhs, const ezExposedPrefabParameterDesc& rhs) -> bool {
    return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash();
  });

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
  AddAmbientLight(false);
}

void ezSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}


void ezSceneContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
  ezGameStateBase* pState = GetGameState();
  if (pState && pState->WasQuitRequested())
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->DeactivateGameState();

    ezGameModeMsgToEditor msgToEd;
    msgToEd.m_DocumentGuid = GetDocumentGuid();
    msgToEd.m_bRunningPTG = false;

    SendProcessMessage(&msgToEd);
  }
}

bool ezSceneContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  const ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld.Borrow());

  ezSceneViewContext* pMaterialViewContext = static_cast<ezSceneViewContext*>(pThumbnailViewContext);
  const bool result = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void ezSceneContext::AddAmbientLight(bool bSetEditorTag)
{
  if (!m_hAmbientLight[0].IsInvalidated())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  const ezColorGammaUB ambient[3] = {ezColor::White, ezColor::White, ezColor::White};
  const float intensity[3] = {10, 5, 3};

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("Ambient Light");

    /// \todo These settings are crap, but I don't care atm
    if (i == 0)
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));
    if (i == 1)
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(1.0f, 0.0f, 0.0f), ezAngle::Degree(30.0f));
    if (i == 2)
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(220.0f));

    if (bSetEditorTag)
    {
      const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    ezGameObject* pLight;
    m_hAmbientLight[i] = GetWorld()->CreateObject(obj, pLight);

    ezDirectionalLightComponent* pDirLight = nullptr;
    ezDirectionalLightComponent::CreateComponent(pLight, pDirLight);
    pDirLight->SetLightColor(ambient[i]);
    pDirLight->SetIntensity(intensity[i]);

    if (i == 0)
    {
      pDirLight->SetCastShadows(true);
    }
  }

  // the actual ambient light component is dangerous to add, because it is a singleton and you cannot have more than one in a scene
  // which means if the user added one, this makes trouble
  // also the Remove/Add pattern doesn't work, because components are always delayed deleted, and the singleton lives longer than it should
  // ezAmbientLightComponent* pAmbLight = nullptr;
  // ezAmbientLightComponent::CreateComponent(GetWorld(), pAmbLight);
  // if (pAmbLight != nullptr)
  //{
  //  pLight->AttachComponent(pAmbLight);
  //}
}

void ezSceneContext::RemoveAmbientLight()
{
  if (m_hAmbientLight[0].IsInvalidated())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hAmbientLight[i]);
    m_hAmbientLight[i].Invalidate();
  }
}

void ezSceneContext::HandleExposedPropertiesMsg(const ezExposedDocumentObjectPropertiesMsgToEngine* pMsg)
{
  m_ExposedSceneProperties = pMsg->m_Properties;
}

void ezSceneContext::HandleSceneGeometryMsg(const ezExportSceneGeometryMsgToEngine* pMsg)
{
  ezWorldGeoExtractionUtil::Geometry geo;

  ezTagSet excludeTags;
  excludeTags.SetByName("Editor");

  if (pMsg->m_bSelectionOnly)
    ezWorldGeoExtractionUtil::ExtractWorldGeometry(
        geo, *m_pWorld, static_cast<ezWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), m_SelectionWithChildren);
  else
    ezWorldGeoExtractionUtil::ExtractWorldGeometry(
        geo, *m_pWorld, static_cast<ezWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), &excludeTags);

  ezWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(pMsg->m_sOutputFile, geo, pMsg->m_Transform);
}

void ezSceneContext::HandlePullObjectStateMsg(const ezPullObjectStateMsgToEngine* pMsg)
{
  if (!m_pWorld->GetWorldSimulationEnabled())
    return;

  const ezWorld* pWorld = GetWorld();
  EZ_LOCK(pWorld->GetReadMarker());

  const auto& objectMapper = GetDocumentContext(GetDocumentGuid())->m_Context.m_GameObjectMap;

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

      state.m_ObjectGuid = objectGuid;
      state.m_bAdjustFromPrefabRootChild = bAdjust;
      state.m_vPosition = pObject->GetGlobalPosition();
      state.m_qRotation = pObject->GetGlobalRotation();
    }
  }

  // the return message is sent after the simulation has stopped
}
