#include <PCH.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectContextDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezGameObjectContextDocument::ezGameObjectContextDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection, bool bUseIPCObjectMirror)
  : ezGameObjectDocument(szDocumentPath, pObjectManager, bUseEngineConnection, bUseIPCObjectMirror)
{

}

ezGameObjectContextDocument::~ezGameObjectContextDocument()
{

}

ezStatus ezGameObjectContextDocument::SetContext(ezUuid documentGuid, ezUuid objectGuid)
{
  const ezAbstractObjectGraph* pPrefab = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(documentGuid);
  if (!pPrefab)
    return ezStatus("Context document could not be loaded.");

  {
    ezGameObjectContextEvent e;
    e.m_Type = ezGameObjectContextEvent::Type::ContextAboutToBeChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  ClearContext();
  ezAbstractObjectGraph graph;
  pPrefab->Clone(graph);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);
  ezDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  {
    EZ_PROFILE("Restoring Objects");
    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    EZ_ASSERT_DEV(pRootNode->FindProperty("TempObjects") == nullptr, "TempObjects should not be serialized.");
    pRootNode->RenameProperty("Children", "TempObjects");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());
  }
  {
    EZ_PROFILE("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph, false);
  }
  {
    ezGameObjectContextEvent e;
    e.m_Type = ezGameObjectContextEvent::Type::ContextChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  return ezStatus(EZ_SUCCESS);
}

ezUuid ezGameObjectContextDocument::GetContextDocument() const
{
  return m_ContextDocument;
}

ezUuid ezGameObjectContextDocument::GetContextObject() const
{
  return m_ContextObject;

}

void ezGameObjectContextDocument::ClearContext()
{
  m_ContextDocument = ezUuid();
  ezDocumentObject* pRoot = GetObjectManager()->GetRootObject();
  ezHybridArray<ezVariant, 16> values;
  GetObjectAccessor()->GetValues(pRoot, "TempObjects", values);
  for (ezInt32 i = (ezInt32)values.GetCount() - 1; i >= 0; --i)
  {
    ezDocumentObject* pChild = GetObjectManager()->GetObject(values[i].Get<ezUuid>());
    GetObjectManager()->RemoveObject(pChild);
    GetObjectManager()->DestroyObject(pChild);
  }
}
