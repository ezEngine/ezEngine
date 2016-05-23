#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Logging/Log.h>
#include <Gizmos/GizmoHandle.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineProcessDocumentContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;


void ezWorldRttiConverterContext::Clear()
{
  ezRttiConverterContext::Clear();

  m_pWorld = nullptr;
  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();

  m_OtherPickingMap.Clear();
  m_ComponentPickingMap.Clear();
}

void* ezWorldRttiConverterContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    ezGameObjectDesc d;
    d.m_sName.Assign(ezConversionUtils::ToString(guid).GetData());

    ezGameObjectHandle hObject = m_pWorld->CreateObject(d);
    ezGameObject* pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      RegisterObject(guid, pRtti, pObject);
      return pObject;
    }
    else
    {
      ezLog::Error("Failed to create ezGameObject!");
      return nullptr;
    }
  }
  else if (pRtti->IsDerivedFrom<ezComponent>())
  {
    ezComponentManagerBase* pMan = m_pWorld->GetOrCreateComponentManager(pRtti);
    if (pMan == nullptr)
    {
      ezLog::Error("Component of type '%s' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return nullptr;
    }
    ezComponentHandle hComponent = pMan->AllocateComponent();
    ezComponent* pComponent;
    if (pMan->TryGetComponent(hComponent, pComponent))
    {
      RegisterObject(guid, pRtti, pComponent);
      return pComponent;
    }
    else
    {
      ezLog::Error("Component of type '%s' cannot be found after creation", pRtti->GetTypeName());
      return nullptr;
    }
  }
  else
  {
    return ezRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void ezWorldRttiConverterContext::DeleteObject(const ezUuid& guid)
{
  ezRttiConverterObject object = GetObjectByGUID(guid);
  const ezRTTI* pRtti = object.m_pType;
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");
  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    UnregisterObject(guid);
    m_pWorld->DeleteObjectNow(hObject);
  }
  else if (pRtti->IsDerivedFrom<ezComponent>())
  {
    ezComponentHandle hComponent = m_ComponentMap.GetHandle(guid);
    ezComponentManagerBase* pMan = m_pWorld->GetOrCreateComponentManager(pRtti);
    if (pMan == nullptr)
    {
      ezLog::Error("Component of type '%s' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return;
    }

    UnregisterObject(guid);
    pMan->DeleteComponent(hComponent);
  }
  else
  {
    ezRttiConverterContext::DeleteObject(guid);
  }
}

void ezWorldRttiConverterContext::RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject)
{
  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    ezGameObject* pGameObject = static_cast<ezGameObject*>(pObject);
    m_GameObjectMap.RegisterObject(guid, pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<ezComponent>())
  {
    ezComponent* pComponent = static_cast<ezComponent*>(pObject);
    m_ComponentMap.RegisterObject(guid, pComponent->GetHandle());
    pComponent->SetEditorPickingID(m_uiNextComponentPickingID++);
    m_ComponentPickingMap.RegisterObject(guid, pComponent->GetEditorPickingID());
  }
  
  ezRttiConverterContext::RegisterObject(guid, pRtti, pObject);
}

void ezWorldRttiConverterContext::UnregisterObject(const ezUuid& guid)
{
  ezRttiConverterObject object = GetObjectByGUID(guid);
  EZ_ASSERT_DEBUG(object.m_pObject, "Failed to retrieve object by guid!");
  const ezRTTI* pRtti = object.m_pType;
  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    m_GameObjectMap.UnregisterObject(guid);
  }
  else if (pRtti->IsDerivedFrom<ezComponent>())
  {
    m_ComponentMap.UnregisterObject(guid);
    m_ComponentPickingMap.UnregisterObject(guid);
  }

  ezRttiConverterContext::UnregisterObject(guid);
}

ezRttiConverterObject ezWorldRttiConverterContext::GetObjectByGUID(const ezUuid& guid) const
{
  ezRttiConverterObject object = ezRttiConverterContext::GetObjectByGUID(guid);

  // We can't look up the ptr via the base class map as it keeps changing, we we need to use the handle.
  if (object.m_pType == ezGetStaticRTTI<ezGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    ezGameObject* pGameObject = nullptr;
    if (!m_pWorld->TryGetObject(hObject, pGameObject))
    {
      EZ_REPORT_FAILURE("Can't resolve game object GUID!");
    }

    // Update new ptr of game object
    if (object.m_pObject != pGameObject)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pGameObject;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  else if (object.m_pType->IsDerivedFrom<ezComponent>())
  {
    auto hComponent = m_ComponentMap.GetHandle(guid);
    ezComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      EZ_REPORT_FAILURE("Can't resolve component GUID!");
    }

    // Update new ptr of component
    if (object.m_pObject != pComponent)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pComponent;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  return object;
}

ezUuid ezWorldRttiConverterContext::GetObjectGUID(const ezRTTI* pRtti, void* pObject) const
{
  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    ezGameObject* pGameObject = static_cast<ezGameObject*>(pObject);
    return m_GameObjectMap.GetGuid(pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<ezComponent>())
  {
    ezComponent* pComponent = static_cast<ezComponent*>(pObject);
    return m_ComponentMap.GetGuid(pComponent->GetHandle());
  }
  return ezRttiConverterContext::GetObjectGUID(pRtti, pObject);
}


ezEngineProcessDocumentContext* ezEngineProcessDocumentContext::GetDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void ezEngineProcessDocumentContext::AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pContext, ezProcessCommunication* pIPC)
{
  EZ_ASSERT_DEV(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
  s_DocumentContexts[guid] = pContext;

  pContext->Initialize(guid, pIPC);
}

void ezEngineProcessDocumentContext::DestroyDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pContext = nullptr;
  if (s_DocumentContexts.Remove(guid, &pContext))
  {
    pContext->Deinitialize(true);
    pContext->GetDynamicRTTI()->GetAllocator()->Deallocate(pContext);
  }
}




ezEngineProcessDocumentContext::ezEngineProcessDocumentContext()
{
  m_pWorld = nullptr;
}

ezEngineProcessDocumentContext::~ezEngineProcessDocumentContext()
{
  EZ_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'ezEngineProcessDocumentContext::DestroyDocumentContext'");
}

void ezEngineProcessDocumentContext::Initialize(const ezUuid& DocumentGuid, ezProcessCommunication* pIPC)
{
  m_DocumentGuid = DocumentGuid;
  m_pIPC = pIPC;

  m_pWorld = ezGameApplication::GetGameApplicationInstance()->CreateWorld(ezConversionUtils::ToString(m_DocumentGuid), true);

  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  OnInitialize();
}

void ezEngineProcessDocumentContext::Deinitialize(bool bFullDestruction)
{
  //if (bFullDestruction)
    ClearViewContexts();
  m_Mirror.Clear();
  m_Mirror.DeInit();
  m_Context.Clear();

  OnDeinitialize();

  CleanUpContextSyncObjects();

  ezGameApplication::GetGameApplicationInstance()->DestroyWorld(m_pWorld);
  m_pWorld = nullptr;
}

void ezEngineProcessDocumentContext::SendProcessMessage(ezProcessMessage* pMsg /*= false*/)
{
  m_pIPC->SendMessage(pMsg);
}

void ezEngineProcessDocumentContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEntityMsgToEngine>())
  {
    const ezEntityMsgToEngine* pMsg2 = static_cast<const ezEntityMsgToEngine*>(pMsg);
    m_Mirror.ApplyOp(const_cast<ezObjectChange&>(pMsg2->m_change));
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineSyncObjectMsg>())
  {
    const ezEditorEngineSyncObjectMsg* pMsg2 = static_cast<const ezEditorEngineSyncObjectMsg*>(pMsg);

    ProcessEditorEngineSyncObjectMsg(*pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectTagMsgToEngine>())
  {
    const ezObjectTagMsgToEngine* pMsg2 = static_cast<const ezObjectTagMsgToEngine*>(pMsg);

    ezGameObjectHandle hObject = m_Context.m_GameObjectMap.GetHandle(pMsg2->m_ObjectGuid);

    ezTag tag;
    ezTagRegistry::GetGlobalRegistry().RegisterTag(pMsg2->m_sTag, &tag);

    ezGameObject* pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      if (pMsg2->m_bSetTag)
        pObject->GetTags().Set(tag);
      else
        pObject->GetTags().Remove(tag);
    }
  }
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezExportSceneMsgToEngine>())
  {
    const ezExportSceneMsgToEngine* pMsg2 = static_cast<const ezExportSceneMsgToEngine*>(pMsg);

    ExportScene(pMsg2);
  }


  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineViewMsg>())
  {
    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
    {
      UpdateSyncObjects();
    }

    const ezEditorEngineViewMsg* pViewMsg = static_cast<const ezEditorEngineViewMsg*>(pMsg);
    EZ_ASSERT_DEV(pViewMsg->m_uiViewID < 0xFFFFFFFF, "Invalid view ID in '%s'", pMsg->GetDynamicRTTI()->GetTypeName());

    if (pViewMsg->m_uiViewID >= m_ViewContexts.GetCount())
      m_ViewContexts.SetCount(pViewMsg->m_uiViewID + 1);

    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewDestroyedMsgToEngine>())
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] != nullptr)
      {
        DestroyViewContext(m_ViewContexts[pViewMsg->m_uiViewID]);
        m_ViewContexts[pViewMsg->m_uiViewID] = nullptr;

        ezLog::Info("Destroyed View %i", pViewMsg->m_uiViewID);
      }
    }
    else
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] == nullptr)
        m_ViewContexts[pViewMsg->m_uiViewID] = CreateViewContext();

      m_ViewContexts[pViewMsg->m_uiViewID]->HandleViewMessage(pViewMsg);
    }

    return;
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewHighlightMsgToEngine>())
  {
    const ezViewHighlightMsgToEngine* pMsg2 = static_cast<const ezViewHighlightMsgToEngine*>(pMsg);

    m_Context.m_uiHighlightID = m_Context.m_ComponentPickingMap.GetHandle(pMsg2->m_HighlightObject);

    if (m_Context.m_uiHighlightID == 0)
      m_Context.m_uiHighlightID = m_Context.m_OtherPickingMap.GetHandle(pMsg2->m_HighlightObject);
  }
}

void ezEngineProcessDocumentContext::AddSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_SyncObjects[pSync->GetGuid()] = pSync;
}

void ezEngineProcessDocumentContext::RemoveSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_SyncObjects.Remove(pSync->GetGuid());
}

ezEditorEngineSyncObject* ezEngineProcessDocumentContext::FindSyncObject(const ezUuid& guid)
{
  auto it = m_SyncObjects.Find(guid);
  if (it.IsValid())
    return it.Value();
  return nullptr;
}

void ezEngineProcessDocumentContext::ClearViewContexts()
{
  for (auto* pContext : m_ViewContexts)
  {
    DestroyViewContext(pContext);
  }

  m_ViewContexts.Clear();
}


void ezEngineProcessDocumentContext::CleanUpContextSyncObjects()
{
  while (!m_SyncObjects.IsEmpty())
  {
    auto it = m_SyncObjects.GetIterator();
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }
}

void ezEngineProcessDocumentContext::ExportScene(const ezExportSceneMsgToEngine* pMsg)
{
  ezExportSceneMsgToEditor ret;
  ret.m_DocumentGuid = pMsg->m_DocumentGuid;

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

    ezTag tagEditor;
    ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor", &tagEditor);
    ezTagSet tags;
    tags.Set(tagEditor);

    ezWorldWriter ww;
    ww.Write(file, *m_pWorld, &tags);

    ret.m_bSuccess = true;
  }

  // do the actual file writing
  ret.m_bSuccess = file.Close().Succeeded();

  if (!ret.m_bSuccess)
  {
    ezLog::Error("Could not export to file '%s'", pMsg->m_sOutputFile.GetData());
  }

  SendProcessMessage(&ret);
}

void ezEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg)
{
  auto it = m_SyncObjects.Find(msg.m_ObjectGuid);

  if (msg.m_sObjectType.IsEmpty())
  {
    // object has been deleted!
    if (it.IsValid())
    {
      it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
    }

    return;
  }

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(msg.m_sObjectType);
  ezEditorEngineSyncObject* pSyncObject = nullptr;
  bool bSetOwner = false;

  if (pRtti == nullptr)
  {
    ezLog::Error("Cannot sync object of type unknown '%s' to engine process", msg.m_sObjectType.GetData());
    return;
  }

  if (!it.IsValid())
  {
    // object does not yet exist
    EZ_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '%s' does not have a default allocator", msg.m_sObjectType.GetData());
    void* pObject = pRtti->GetAllocator()->Allocate();

    pSyncObject = static_cast<ezEditorEngineSyncObject*>(pObject);
    bSetOwner = true;
  }
  else
  {
    pSyncObject = it.Value();
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);
  writer.WriteBytes(msg.m_ObjectData.GetData(), msg.m_ObjectData.GetCount());

  ezReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, pSyncObject);

  if (bSetOwner)
    pSyncObject->SetOwner(this);

  pSyncObject->SetModified(true);
}


void ezEngineProcessDocumentContext::Reset()
{
  ezUuid guid = m_DocumentGuid;
  auto ipc = m_pIPC;

  Deinitialize(false);

  Initialize(guid, ipc);
}
void ezEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto* pSyncObject : m_SyncObjects)
  {
    if (pSyncObject->GetModified() && pSyncObject->GetDynamicRTTI()->IsDerivedFrom<ezEngineGizmoHandle>())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      ezEngineGizmoHandle* pGizmoHandle = static_cast<ezEngineGizmoHandle*>(pSyncObject);

      EZ_LOCK(m_pWorld->GetWriteMarker());

      if (pGizmoHandle->SetupForEngine(m_pWorld, m_Context.m_uiNextComponentPickingID))
      {
        m_Context.m_OtherPickingMap.RegisterObject(pGizmoHandle->GetGuid(), m_Context.m_uiNextComponentPickingID);
        ++m_Context.m_uiNextComponentPickingID;
      }

      pGizmoHandle->UpdateForEngine(m_pWorld);
    }
  }
}

