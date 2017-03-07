#include <PCH.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Serialization/DdlSerializer.h>

EZ_IMPLEMENT_SINGLETON(ezPrefabCache);

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ezPrefabCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezPrefabCache);
  }

  ON_CORE_SHUTDOWN
  {
    ezPrefabCache* pDummy = ezPrefabCache::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION


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
  return &data.m_Graph;
}

void ezPrefabCache::LoadGraph(ezAbstractObjectGraph& out_graph, ezStringView sGraph)
{
  ezUInt64 uiHash = ezHashing::MurmurHash64(sGraph.GetData(), sGraph.GetElementCount());
  auto it = m_CachedGraphs.Find(uiHash);
  if (!it.IsValid())
  {
    it = m_CachedGraphs.Insert(uiHash, ezUniquePtr<ezAbstractObjectGraph>(EZ_DEFAULT_NEW(ezAbstractObjectGraph)));
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter stringWriter(&storage);
    ezMemoryStreamReader stringReader(&storage);
    stringWriter.WriteBytes(sGraph.GetData(), sGraph.GetElementCount());

    ezAbstractGraphDdlSerializer::Read(stringReader, it.Value().Borrow());
  }

  it.Value()->Clone(out_graph);
}

ezPrefabCache::PrefabData& ezPrefabCache::GetOrCreatePrefabCache(const ezUuid& documentGuid)
{
  auto it = m_PrefabData.Find(documentGuid);
  if (it.IsValid())
  {
    ezFileStats Stats;
    if (ezOSFile::GetFileStats(it.Value()->m_sAbsPath, Stats).Succeeded() && !Stats.m_LastModificationTime.Compare(it.Value()->m_fileModifiedTime, ezTimestamp::CompareMode::FileTimeEqual))
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
      ezLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid.GetData());
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
      ezLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid.GetData());
      return;
    }
  }

  ezFileStats Stats;
  bool bStat = ezOSFile::GetFileStats(data.m_sAbsPath, Stats).Succeeded();

  if (!bStat)
  {
    ezLog::Error("Can't update prefab file '{0}', the file can't be opened.", data.m_sAbsPath.GetData());
    return;
  }

  data.m_sDocContent = ezPrefabUtils::ReadDocumentAsString(data.m_sAbsPath);

  if (data.m_sDocContent.IsEmpty())
    return;

  data.m_fileModifiedTime = Stats.m_LastModificationTime;
  data.m_Graph.Clear();
  ezPrefabUtils::LoadGraph(data.m_Graph, data.m_sDocContent);

}
