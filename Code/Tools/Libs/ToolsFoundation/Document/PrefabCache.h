#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/UniquePtr.h>

class ezAbstractObjectGraph;

class EZ_TOOLSFOUNDATION_DLL ezPrefabCache
{
  EZ_DECLARE_SINGLETON(ezPrefabCache);

public:
  ezPrefabCache();

  const ezStringBuilder& GetCachedPrefabDocument(const ezUuid& documentGuid);
  const ezAbstractObjectGraph* GetCachedPrefabGraph(const ezUuid& documentGuid);
  void LoadGraph(ezAbstractObjectGraph& out_graph, ezStringView sGraph);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ezPrefabCache);

  struct PrefabData
  {
    PrefabData() {}

    ezUuid m_documentGuid;
    ezString m_sAbsPath;

    ezAbstractObjectGraph m_Graph;
    ezStringBuilder m_sDocContent;
    ezTimestamp m_fileModifiedTime;

  };
  PrefabData& GetOrCreatePrefabCache(const ezUuid& documentGuid);
  void UpdatePrefabData(PrefabData& data);

  ezMap<ezUInt64, ezUniquePtr<ezAbstractObjectGraph>> m_CachedGraphs;
  ezMap<ezUuid, ezUniquePtr<PrefabData>> m_PrefabData;

};
