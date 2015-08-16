#include <PCH.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezEditorEngineSyncObject);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSyncObject, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezHashTable<ezUuid, ezEditorEngineSyncObject*> ezEditorEngineSyncObject::s_AllSyncObjects;
ezMap<ezUuid, ezHybridArray<ezUuid, 32> > ezEditorEngineSyncObject::s_DeletedObjects;

ezEditorEngineSyncObject::ezEditorEngineSyncObject()
{
  m_SyncObjectGuid.CreateNewUuid();
  m_bModified = true;

  s_AllSyncObjects[m_SyncObjectGuid] = this;
}

ezEditorEngineSyncObject::~ezEditorEngineSyncObject()
{
  s_DeletedObjects[m_DocumentGuid].PushBack(m_SyncObjectGuid);
  s_AllSyncObjects.Remove(m_SyncObjectGuid);
}

void ezEditorEngineSyncObject::ChangeObjectGuid(const ezUuid& guid)
{
  s_AllSyncObjects.Remove(m_SyncObjectGuid);
  m_SyncObjectGuid = guid;
  s_AllSyncObjects[m_SyncObjectGuid] = this;
}

void ezEditorEngineSyncObject::SyncObjectsToEngine(ezEditorEngineConnection& connection, bool bSyncAll)
{
  const ezUuid DocumentGuid = connection.GetDocument()->GetGuid();

  // Tell the engine which sync objects have been removed recently
  {
    auto& Deleted = s_DeletedObjects[DocumentGuid];

    for (ezUInt32 i = 0; i < Deleted.GetCount(); ++i)
    {
      ezEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = Deleted[i];
      connection.SendMessage(&msg);
    }

    Deleted.Clear();
  }

  ezEditorEngineSyncObject* pObject = ezEditorEngineSyncObject::GetFirstInstance();

  while (pObject)
  {
    EZ_ASSERT_DEV(pObject->m_DocumentGuid.IsValid(), "Document GUID is not set on object of type %s", pObject->GetDynamicRTTI()->GetTypeName());

    if ((bSyncAll || pObject->GetModified()) && (DocumentGuid == pObject->m_DocumentGuid))
    {
      ezEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = pObject->m_SyncObjectGuid;
      msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

      ezMemoryStreamStorage storage;
      ezMemoryStreamWriter writer(&storage);
      ezMemoryStreamReader reader(&storage);

      ezReflectionSerializer::WriteObjectToJSON(writer, pObject->GetDynamicRTTI(), pObject);

      ezStringBuilder sData;
      sData.ReadAll(reader);

      msg.SetObjectData(sData);

      connection.SendMessage(&msg);

      pObject->SetModified(false);
    }

    pObject = pObject->GetNextInstance();
  }
}

ezEditorEngineSyncObject* ezEditorEngineSyncObject::FindSyncObject(const ezUuid & guid)
{
  ezEditorEngineSyncObject* pRes = nullptr;
  s_AllSyncObjects.TryGetValue(guid, pRes);
  return pRes;
}

