#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;

ezEngineProcessDocumentContext* ezEngineProcessDocumentContext::GetDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void ezEngineProcessDocumentContext::AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView)
{
  EZ_ASSERT(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
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

