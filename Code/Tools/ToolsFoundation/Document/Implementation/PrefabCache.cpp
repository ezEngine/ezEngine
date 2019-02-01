#include <PCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_IMPLEMENT_SINGLETON(ezPrefabCache);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ezPrefabCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezPrefabCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPrefabCache* pDummy = ezPrefabCache::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezPrefabCache::ezPrefabCache()
    : m_SingletonRegistrar(this)
{
}

const ezStringBuilder& ezPrefabCache::GetCachedPrefabDocument(const ezUuid& documentGuid)
{
  PrefabData& data = ezPrefabCache::GetOrCreatePrefabCache(documentGuid);
  return data.m_sDocContent;
}

const ezAbstractObjectGraph* ezPrefabCache::GetCachedPrefabGraph(const ezUuid& documentGuid)
{
  PrefabData& data = ezPrefabCache::GetOrCreatePrefabCache(documentGuid);
  if (data.m_sAbsPath.IsEmpty())
    return nullptr;
  return &data.m_Graph;
}

void ezPrefabCache::LoadGraph(ezAbstractObjectGraph& out_graph, ezStringView sGraph)
{
  ezUInt64 uiHash = ezHashingUtils::xxHash64(sGraph.GetData(), sGraph.GetElementCount());
  auto it = m_CachedGraphs.Find(uiHash);
  if (!it.IsValid())
  {
    it = m_CachedGraphs.Insert(uiHash, ezUniquePtr<ezAbstractObjectGraph>(EZ_DEFAULT_NEW(ezAbstractObjectGraph)));

    ezRawMemoryStreamReader stringReader(sGraph.GetData(), sGraph.GetElementCount());
    ezUniquePtr<ezAbstractObjectGraph> header;
    ezUniquePtr<ezAbstractObjectGraph> types;
    ezAbstractGraphDdlSerializer::ReadDocument(stringReader, header, it.Value(), types, true);
  }

  it.Value()->Clone(out_graph);
}

ezPrefabCache::PrefabData& ezPrefabCache::GetOrCreatePrefabCache(const ezUuid& documentGuid)
{
  auto it = m_PrefabData.Find(documentGuid);
  if (it.IsValid())
  {
    ezFileStats Stats;
    if (ezOSFile::GetFileStats(it.Value()->m_sAbsPath, Stats).Succeeded() &&
        !Stats.m_LastModificationTime.Compare(it.Value()->m_fileModifiedTime, ezTimestamp::CompareMode::FileTimeEqual))
    {
      UpdatePrefabData(*it.Value().Borrow());
    }
  }
  else
  {
    it = m_PrefabData.Insert(documentGuid, ezUniquePtr<PrefabData>(EZ_DEFAULT_NEW(PrefabData)));

    it.Value()->m_documentGuid = documentGuid;
    it.Value()->m_sAbsPath = ezToolsProject::GetSingleton()->GetPathForDocumentGuid(documentGuid);
    if (it.Value()->m_sAbsPath.IsEmpty())
    {
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(documentGuid, sGuid);
      ezLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
    }
    else
      UpdatePrefabData(*it.Value().Borrow());
  }

  return *it.Value().Borrow();
}

void ezPrefabCache::UpdatePrefabData(PrefabData& data)
{
  if (data.m_sAbsPath.IsEmpty())
  {
    data.m_sAbsPath = ezToolsProject::GetSingleton()->GetPathForDocumentGuid(data.m_documentGuid);
    if (data.m_sAbsPath.IsEmpty())
    {
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(data.m_documentGuid, sGuid);
      ezLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
      return;
    }
  }

  ezFileStats Stats;
  bool bStat = ezOSFile::GetFileStats(data.m_sAbsPath, Stats).Succeeded();

  if (!bStat)
  {
    ezLog::Error("Can't update prefab file '{0}', the file can't be opened.", data.m_sAbsPath);
    return;
  }

  data.m_sDocContent = ezPrefabUtils::ReadDocumentAsString(data.m_sAbsPath);

  if (data.m_sDocContent.IsEmpty())
    return;

  data.m_fileModifiedTime = Stats.m_LastModificationTime;
  data.m_Graph.Clear();
  ezPrefabUtils::LoadGraph(data.m_Graph, data.m_sDocContent);
}
