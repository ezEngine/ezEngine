#include <PCH.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <Foundation/Configuration/Singleton.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, 1, ezRTTIDefaultAllocator<ezSceneContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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

void ezSceneContext::DrawSelectionBounds()
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
      ezDebugRenderer::DrawLineBoxCorners(m_pWorld, bounds.GetBox(), 0.25f, ezColor::Yellow);
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
}

ezSceneContext::~ezSceneContext()
{
  ezVisualScriptComponent::s_ActivityEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneContext::OnVisualScriptActivity, this));
}

void ezSceneContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezSceneSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages
    HandleSceneSettingsMsg(static_cast<const ezSceneSettingsMsgToEngine*>(pMsg));
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

  if (pMsg->IsInstanceOf<ezViewRedrawMsgToEngine>())
  {
    /// \todo We are actually doing this once per view, but the data is going to be rendered in every view
    /// That means we in 4-view mode we render this stuff 3 times more than necessary
    DrawSelectionBounds();

    // fall through
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}


void ezSceneContext::HandleSceneSettingsMsg(const ezSceneSettingsMsgToEngine* pMsg)
{
  ezGizmoRenderer::s_fGizmoScale = pMsg->m_fGizmoScale;

  const bool bSimulate = pMsg->m_bSimulateWorld;
  m_bRenderSelectionOverlay = pMsg->m_bRenderOverlay;
  m_bRenderShapeIcons = pMsg->m_bRenderShapeIcons;
  m_bRenderSelectionBoxes = pMsg->m_bRenderSelectionBoxes;
  m_fGridDensity = pMsg->m_fGridDensity;

  if (m_fGridDensity != 0.0f)
  {
    m_GridTransform.m_vPosition = pMsg->m_vGridCenter;

    if (pMsg->m_vGridTangent1.IsZero())
    {
      m_GridTransform.m_Rotation.SetZero();
    }
    else
    {
      m_GridTransform.m_Rotation.SetColumn(0, pMsg->m_vGridTangent1);
      m_GridTransform.m_Rotation.SetColumn(1, pMsg->m_vGridTangent2);
      m_GridTransform.m_Rotation.SetColumn(2, pMsg->m_vGridTangent1.Cross(pMsg->m_vGridTangent2));
    }
  }

  ezGameState* pState = GetGameState();
  m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);

  if (pState == nullptr && bSimulate != m_pWorld->GetWorldSimulationEnabled())
  {
    m_pWorld->SetWorldSimulationEnabled(bSimulate);

    if (bSimulate)
      OnSimulationEnabled();
    else
      OnSimulationDisabled();
  }

  if (pMsg->m_bAddAmbientLight)
    AddAmbientLight(true);
  else
    RemoveAmbientLight();

  if (pState && pState->WasQuitRequested())
  {
    ezGameApplication::GetGameApplicationInstance()->DeactivateGameStateForWorld(m_pWorld);
    ezGameApplication::GetGameApplicationInstance()->DestroyGameStateForWorld(m_pWorld);

    ezGameModeMsgToEditor msgToEd;
    msgToEd.m_DocumentGuid = pMsg->m_DocumentGuid;
    msgToEd.m_bRunningPTG = false;

    SendProcessMessage(&msgToEd);
  }
}

void ezSceneContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    EZ_LOCK(m_pWorld->GetReadMarker());

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

  //EZ_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

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

  ezResourceManager::ReloadAllResources(false);

  ezGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(true);
  }
}

void ezSceneContext::OnSimulationDisabled()
{
  ezLog::Info("World Simulation disabled");

  ezResourceManager::ResetAllResources();

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

ezGameState* ezSceneContext::GetGameState() const
{
  return ezGameApplication::GetGameApplicationInstance()->GetGameStateForWorld(m_pWorld);
}

void ezSceneContext::OnInitialize()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());
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

void ezSceneContext::OnPlayTheGameModeStarted()
{
  ezLog::Info("Starting Play-the-Game mode");

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  ezGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  ezGameApplication::GetGameApplicationInstance()->CreateGameStateForWorld(m_pWorld);
  ezGameApplication::GetGameApplicationInstance()->ActivateGameStateForWorld(m_pWorld);

  ezGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG = true;

  SendProcessMessage(&msgRet);

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


void ezSceneContext::HandleObjectsForDebugVisMsg(const ezObjectsForDebugVisMsgToEngine* pMsg)
{
  EZ_LOCK(GetWorld()->GetWriteMarker());

  const ezArrayPtr<const ezUuid> guids(reinterpret_cast<const ezUuid*>(pMsg->m_Objects.GetData()), pMsg->m_Objects.GetCount() / sizeof(ezUuid));

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
  ezGameState* pState = GetGameState();

  if (pMsg->m_bEnablePTG)
  {
    if (pState != nullptr)
    {
      ezLog::Error("Cannot start Play-the-Game, there is already a game state active for this world");
      return;
    }

    ezResourceManager::ReloadAllResources(false);
    OnPlayTheGameModeStarted();
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
    ww.Write(file, *m_pWorld, &tags);
  }

  // do the actual file writing
  return file.Close().Succeeded();
}

void ezSceneContext::OnThumbnailViewContextCreated()
{
  // make sure there is ambient light in the thumbnails
  // should check whether this is a prefab
  RemoveAmbientLight();
  AddAmbientLight(false);
}

void ezSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}

bool ezSceneContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  const ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  ezSceneViewContext* pMaterialViewContext = static_cast<ezSceneViewContext*>(pThumbnailViewContext);
  const bool result = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void ezSceneContext::AddAmbientLight(bool bSetEditorTag)
{
  if (!m_hAmbientLight[0].IsInvalidated())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  const ezColorGammaUB ambient[3] = { ezColor::White, ezColor::White, ezColor::White };
  const float intensity[3] = { 10, 5, 3 };

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
  //ezAmbientLightComponent* pAmbLight = nullptr;
  //ezAmbientLightComponent::CreateComponent(GetWorld(), pAmbLight);
  //if (pAmbLight != nullptr)
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
    GetWorld()->DeleteObjectDelayed(m_hAmbientLight[i]);
    m_hAmbientLight[i].Invalidate();
  }
}

