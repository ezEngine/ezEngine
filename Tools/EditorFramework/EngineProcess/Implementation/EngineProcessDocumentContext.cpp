#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Reflection/ReflectionUtils.h>
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
}

void ezEngineProcessDocumentContext::DestroyDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  if (s_DocumentContexts.Remove(guid, &pResult))
  {
    EZ_DEFAULT_DELETE(pResult);
  }
}

void ezEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg)
{
  const ezRTTI* pRtti = ezRTTI::FindTypeByName(msg.m_sObjectType);

  ezLog::Debug("Sync object message: '%s'", msg.m_sObjectType.GetData());

  if (pRtti == nullptr)
  {
    ezLog::Error("Cannot sync object of type unknown '%s' to engine process", msg.m_sObjectType.GetData());
    return;
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);
  writer.WriteBytes(msg.m_sObjectData.GetData(), msg.m_sObjectData.GetElementCount());

  ezEditorEngineSyncObject* pSyncObject = nullptr;
  if (!m_pSyncObjects.TryGetValue(msg.m_ObjectGuid, pSyncObject))
  {
    // object does not yet exist
    EZ_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '%s' does not have a default allocator", msg.m_sObjectType.GetData());
    void* pObject = pRtti->GetAllocator()->Allocate();

    pSyncObject = static_cast<ezEditorEngineSyncObject*>(pObject);
    m_pSyncObjects[msg.m_ObjectGuid] = pSyncObject;

    ezLog::Debug("Allocated Sync object '%s'", msg.m_sObjectType.GetData());
  }

  ezReflectionUtils::ReadObjectPropertiesFromJSON(reader, *pRtti, pSyncObject);
}

