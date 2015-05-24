#include <PCH.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Reflection/ReflectionSerializer.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezEditorEngineSyncObject);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSyncObject, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezEditorEngineSyncObject::SyncObjectsToEngine(ezEditorEngineConnection& connection, bool bSyncAll)
{
  ezEditorEngineSyncObject* pObject = ezEditorEngineSyncObject::GetFirstInstance();

  const ezUuid DocumentGuid = connection.GetDocument()->GetGuid();

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

