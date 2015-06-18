#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Reflection/ReflectionSerializer.h>
#include <Foundation/Logging/Log.h>

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;

ezEngineProcessDocumentContext* ezEngineProcessDocumentContext::GetDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void ezEngineProcessDocumentContext::AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView)
{
  EZ_ASSERT_DEV(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
  s_DocumentContexts[guid] = pView;
  pView->m_DocumentGuid = guid;
}

void ezEngineProcessDocumentContext::DestroyDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  if (s_DocumentContexts.Remove(guid, &pResult))
  {
    EZ_DEFAULT_DELETE(pResult);
  }
}

ezEngineProcessDocumentContext::~ezEngineProcessDocumentContext()
{
  CleanUpContextSyncObjects();
}

void ezEngineProcessDocumentContext::CleanUpContextSyncObjects()
{
  ezEditorEngineSyncObject* pSyncObject = ezEditorEngineSyncObject::GetFirstInstance();

  while (pSyncObject)
  {
    ezEditorEngineSyncObject* pNextSyncObject = pSyncObject->GetNextInstance();

    // remove all sync objects that are linked to the same document
    if (pSyncObject->GetDocumentGuid() == m_DocumentGuid)
    {
      pSyncObject->GetDynamicRTTI()->GetAllocator()->Deallocate(pSyncObject);
    }

    pSyncObject = pNextSyncObject;
  }
}

void ezEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg)
{
  ezEditorEngineSyncObject* pSyncObject = ezEditorEngineSyncObject::FindSyncObject(msg.m_ObjectGuid);

  if (msg.m_sObjectType.IsEmpty())
  {
    // object has been deleted!
    if (pSyncObject != nullptr)
    {
      pSyncObject->GetDynamicRTTI()->GetAllocator()->Deallocate(pSyncObject);
    }

    return;
  }

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(msg.m_sObjectType);

  if (pRtti == nullptr)
  {
    ezLog::Error("Cannot sync object of type unknown '%s' to engine process", msg.m_sObjectType.GetData());
    return;
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);
  writer.WriteBytes(msg.m_sObjectData.GetData(), msg.m_sObjectData.GetElementCount());

  if (pSyncObject == nullptr)
  {
    // object does not yet exist
    EZ_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '%s' does not have a default allocator", msg.m_sObjectType.GetData());
    void* pObject = pRtti->GetAllocator()->Allocate();

    pSyncObject = static_cast<ezEditorEngineSyncObject*>(pObject);
    pSyncObject->ChangeObjectGuid(msg.m_ObjectGuid);
    pSyncObject->SetDocumentGuid(msg.m_DocumentGuid);
  }

  ezReflectionSerializer::ReadObjectPropertiesFromJSON(reader, *pRtti, pSyncObject);
  pSyncObject->SetModified(true);
}

