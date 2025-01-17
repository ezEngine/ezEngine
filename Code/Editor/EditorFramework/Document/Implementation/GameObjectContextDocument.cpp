#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Preferences/GameObjectContextPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectContextDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezGameObjectContextDocument::ezGameObjectContextDocument(
  ezStringView sDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType)
  : ezGameObjectDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
}

ezGameObjectContextDocument::~ezGameObjectContextDocument() = default;

ezStatus ezGameObjectContextDocument::SetContext(ezUuid documentGuid, ezUuid objectGuid)
{
  if (!documentGuid.IsValid())
  {
    {
      ezGameObjectContextEvent e;
      e.m_Type = ezGameObjectContextEvent::Type::ContextAboutToBeChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    ClearContext();
    {
      ezGameObjectContextEvent e;
      e.m_Type = ezGameObjectContextEvent::Type::ContextChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    return ezStatus(EZ_SUCCESS);
  }

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
    EZ_PROFILE_SCOPE("Restoring Objects");
    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    EZ_ASSERT_DEV(pRootNode->FindProperty("TempObjects") == nullptr, "TempObjects should not be serialized.");
    pRootNode->RenameProperty("Children", "TempObjects");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());
  }
  {
    EZ_PROFILE_SCOPE("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph, false);
  }
  {
    ezGameObjectContextPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezGameObjectContextPreferencesUser>(this);
    m_ContextDocument = documentGuid;
    pPreferences->SetContextDocument(m_ContextDocument);

    const ezDocumentObject* pContextObject = GetObjectManager()->GetObject(objectGuid);
    m_ContextObject = pContextObject ? objectGuid : ezUuid();
    pPreferences->SetContextObject(m_ContextObject);
  }
  {
    ezGameObjectContextEvent e;
    e.m_Type = ezGameObjectContextEvent::Type::ContextChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  return ezStatus(EZ_SUCCESS);
}

ezUuid ezGameObjectContextDocument::GetContextDocumentGuid() const
{
  return m_ContextDocument;
}

ezUuid ezGameObjectContextDocument::GetContextObjectGuid() const
{
  return m_ContextObject;
}

const ezDocumentObject* ezGameObjectContextDocument::GetContextObject() const
{
  if (m_ContextDocument.IsValid())
  {
    if (m_ContextObject.IsValid())
    {
      return GetObjectManager()->GetObject(m_ContextObject);
    }
    return GetObjectManager()->GetRootObject();
  }
  return nullptr;
}

void ezGameObjectContextDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  ezGameObjectContextPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezGameObjectContextPreferencesUser>(this);
  SetContext(pPreferences->GetContextDocument(), pPreferences->GetContextObject()).LogFailure();
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void ezGameObjectContextDocument::ClearContext()
{
  m_ContextDocument = ezUuid();
  m_ContextObject = ezUuid();
  ezDocumentObject* pRoot = GetObjectManager()->GetRootObject();
  ezHybridArray<ezVariant, 16> values;
  GetObjectAccessor()->GetValuesByName(pRoot, "TempObjects", values).AssertSuccess();
  for (ezInt32 i = (ezInt32)values.GetCount() - 1; i >= 0; --i)
  {
    ezDocumentObject* pChild = GetObjectManager()->GetObject(values[i].Get<ezUuid>());
    GetObjectManager()->RemoveObject(pChild);
    GetObjectManager()->DestroyObject(pChild);
  }
}
