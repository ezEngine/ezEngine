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
#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineProcessDocumentContext, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;

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
    pContext->Deinitialize();
    pContext->GetDynamicRTTI()->GetAllocator()->Deallocate(pContext);
  }
}




ezEngineProcessDocumentContext::ezEngineProcessDocumentContext()
{
  m_pWorld = nullptr;
  m_uiNextComponentPickingID = 1;
}

ezEngineProcessDocumentContext::~ezEngineProcessDocumentContext()
{
  EZ_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'ezEngineProcessDocumentContext::DestroyDocumentContext'");
}

void ezEngineProcessDocumentContext::Deinitialize()
{
  ClearViewContexts();

  OnDeinitialize();

  CleanUpContextSyncObjects();

  EZ_DEFAULT_DELETE(m_pWorld);
}

void ezEngineProcessDocumentContext::SendProcessMessage(ezProcessMessage* pMsg, bool bSuperHighPriority /*= false*/)
{
  m_pIPC->SendMessage(pMsg, bSuperHighPriority);
}

void ezEngineProcessDocumentContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEntityMsgToEngine>())
  {
    const ezEntityMsgToEngine* pMsg2 = static_cast<const ezEntityMsgToEngine*>(pMsg);

    HandlerEntityMsg(pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineSyncObjectMsg>())
  {
    const ezEditorEngineSyncObjectMsg* pMsg2 = static_cast<const ezEditorEngineSyncObjectMsg*>(pMsg);

    ProcessEditorEngineSyncObjectMsg(*pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectTagMsgToEngine>())
  {
    const ezObjectTagMsgToEngine* pMsg2 = static_cast<const ezObjectTagMsgToEngine*>(pMsg);

    ezGameObjectHandle hObject = m_GameObjectMap.GetHandle(pMsg2->m_ObjectGuid);

    ezTag tag;
    ezTagRegistry::GetGlobalRegistry().RegisterTag(pMsg2->m_sTag, &tag);

    ezGameObject* pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      if (pMsg2->m_bSetTag)
        pObject->GetTags().Set(tag);
      else
        pObject->GetTags().Clear(tag);
    }
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

    ezUInt32 uiPickingID = m_OtherPickingMap.GetHandle(pMsg2->m_HighlightObject);

    ezRenderContext::GetDefaultInstance()->SetMaterialParameter("PickingHighlightID", (ezInt32)uiPickingID);
  }

}

void ezEngineProcessDocumentContext::Initialize(const ezUuid& DocumentGuid, ezProcessCommunication* pIPC)
{
  m_DocumentGuid = DocumentGuid;
  m_pIPC = pIPC;

  m_pWorld = EZ_DEFAULT_NEW(ezWorld, ezConversionUtils::ToString(m_DocumentGuid));

  OnInitialize();
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
  writer.WriteBytes(msg.m_sObjectData.GetData(), msg.m_sObjectData.GetElementCount());

  ezReflectionSerializer::ReadObjectPropertiesFromJSON(reader, *pRtti, pSyncObject);

  if (bSetOwner)
    pSyncObject->SetOwner(this);

  pSyncObject->SetModified(true);
}


void ezEngineProcessDocumentContext::HandlerGameObjectMsg(const ezEntityMsgToEngine* pMsg, ezRTTI* pRtti)
{
  switch (pMsg->m_iMsgType)
  {
  case ezEntityMsgToEngine::ObjectAdded:
    {
      ezGameObjectDesc d;
      d.m_sName.Assign(ezConversionUtils::ToString(pMsg->m_ObjectGuid).GetData());

      if (pMsg->m_NewParentGuid.IsValid())
        d.m_hParent = m_GameObjectMap.GetHandle(pMsg->m_NewParentGuid);

      ezGameObjectHandle hObject = m_pWorld->CreateObject(d);
      m_GameObjectMap.RegisterObject(pMsg->m_ObjectGuid, hObject);

      ezGameObject* pObject;
      if (m_pWorld->TryGetObject(hObject, pObject))
      {
        UpdateProperties(pMsg, pObject, ezGetStaticRTTI<ezGameObject>());
      }
    }
    break;

  case ezEntityMsgToEngine::ObjectMoved:
    {
      ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sObjectType);

      if (pRtti == nullptr)
      {
        ezLog::Error("Cannot create object of type '%s', RTTI is unknown", pMsg->m_sObjectType.GetData());
        break;
      }

      ezGameObjectHandle hObject = m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

      ezGameObjectHandle hNewParent;
      if (pMsg->m_NewParentGuid.IsValid())
        hNewParent = m_GameObjectMap.GetHandle(pMsg->m_NewParentGuid);

      ezGameObject* pObject = nullptr;
      if (m_pWorld->TryGetObject(hObject, pObject))
      {
        pObject->SetParent(hNewParent);
      }
      else
        ezLog::Error("Couldn't access game object object %s in world %p", ezConversionUtils::ToString(pMsg->m_ObjectGuid).GetData(), m_pWorld);
    }
    break;

  case ezEntityMsgToEngine::ObjectRemoved:
    {
      m_pWorld->DeleteObject(m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid));
      m_GameObjectMap.UnregisterObject(pMsg->m_ObjectGuid);
    }
    break;

  case ezEntityMsgToEngine::PropertyChanged:
    {
      ezGameObjectHandle hObject = m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

      ezGameObject* pObject;
      if (m_pWorld->TryGetObject(hObject, pObject))
      {
        UpdateProperties(pMsg, pObject, ezGetStaticRTTI<ezGameObject>());
        //pObject->UpdateGlobalTransform();
      }
    }
    break;
  }
}

void ezEngineProcessDocumentContext::HandleComponentMsg(const ezEntityMsgToEngine* pMsg, ezRTTI* pRtti)
{
  ezComponentManagerBase* pMan = m_pWorld->GetComponentManager(pRtti);

  if (pMan == nullptr)
  {
    ezLog::Error("Component of type '%s' cannot be created, no component manager is registered", pRtti->GetTypeName());
    return;
  }

  switch (pMsg->m_iMsgType)
  {
  case ezEntityMsgToEngine::ObjectAdded:
    {
      ezComponentHandle hComponent = pMan->CreateComponent();

      ezGameObjectHandle hParent = m_GameObjectMap.GetHandle(pMsg->m_NewParentGuid);
      ezGameObject* pParent;
      if (!m_pWorld->TryGetObject(hParent, pParent))
        break;

      ezComponent* pComponent;
      if (pMan->TryGetComponent(hComponent, pComponent))
      {
        m_ComponentMap.RegisterObject(pMsg->m_ObjectGuid, hComponent);
        UpdateProperties(pMsg, pComponent, pComponent->GetDynamicRTTI());

        pComponent->m_uiEditorPickingID = m_uiNextComponentPickingID++;

        m_ComponentPickingMap.RegisterObject(pMsg->m_ObjectGuid, pComponent->m_uiEditorPickingID);
      }
      else
      {
        ezLog::Error("Component of type '%s' cannot be found after creation", pRtti->GetTypeName());
      }

      pParent->AddComponent(hComponent);
    }
    break;

  case ezEntityMsgToEngine::ObjectMoved:
    {
      ezGameObjectHandle hParent = m_GameObjectMap.GetHandle(pMsg->m_NewParentGuid);
      ezGameObject* pParent;
      if (!m_pWorld->TryGetObject(hParent, pParent))
        break;

      ezComponentHandle hComponent = m_ComponentMap.GetHandle(pMsg->m_ObjectGuid);

      ezComponent* pComponent;
      if (pMan->TryGetComponent(hComponent, pComponent))
      {
        if (pComponent->GetOwner())
          pComponent->GetOwner()->RemoveComponent(pComponent);
      }

      pParent->AddComponent(hComponent);
    }
    break;

  case ezEntityMsgToEngine::ObjectRemoved:
    {
      ezComponentHandle hComponent = m_ComponentMap.GetHandle(pMsg->m_ObjectGuid);
      m_ComponentMap.UnregisterObject(pMsg->m_ObjectGuid);
      m_ComponentPickingMap.UnregisterObject(pMsg->m_ObjectGuid);

      pMan->DeleteComponent(hComponent);
    }
    break;

  case ezEntityMsgToEngine::PropertyChanged:
    {
      ezComponentHandle hComponent = m_ComponentMap.GetHandle(pMsg->m_ObjectGuid);

      ezComponent* pComponent;
      if (pMan->TryGetComponent(hComponent, pComponent))
      {
        UpdateProperties(pMsg, pComponent, pComponent->GetDynamicRTTI());
      }
      else
      {
        ezLog::Error("Component of type '%s' cannot be found", pRtti->GetTypeName());
      }
    }
    break;
  }

}

void ezEngineProcessDocumentContext::HandlerEntityMsg(const ezEntityMsgToEngine* pMsg)
{

  ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sObjectType);

  if (pRtti == nullptr)
  {
    ezLog::Error("Cannot create object of type '%s', RTTI is unknown", pMsg->m_sObjectType.GetData());
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezGameObject>())
  {
    HandlerGameObjectMsg(pMsg, pRtti);
  }

  if (pRtti->IsDerivedFrom<ezComponent>())
  {
    HandleComponentMsg(pMsg, pRtti);
  }

}

void ezEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto* pSyncObject : m_SyncObjects)
  {
    if (pSyncObject->GetModified() && pSyncObject->GetDynamicRTTI()->IsDerivedFrom<ezGizmoHandle>())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      ezGizmoHandle* pGizmoHandle = static_cast<ezGizmoHandle*>(pSyncObject);

      EZ_LOCK(m_pWorld->GetWriteMarker());

      if (pGizmoHandle->SetupForEngine(m_pWorld, m_uiNextComponentPickingID))
      {
        m_OtherPickingMap.RegisterObject(pGizmoHandle->GetGuid(), m_uiNextComponentPickingID);
        ++m_uiNextComponentPickingID;
      }

      pGizmoHandle->UpdateForEngine(m_pWorld);
    }
  }
}



void ezEngineProcessDocumentContext::UpdateProperties(const ezEntityMsgToEngine* pMsg, void* pObject, const ezRTTI* pRtti)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  writer.WriteBytes(pMsg->m_sObjectData.GetData(), pMsg->m_sObjectData.GetElementCount());

  ezReflectionSerializer::ReadObjectPropertiesFromJSON(reader, *pRtti, pObject);
}




