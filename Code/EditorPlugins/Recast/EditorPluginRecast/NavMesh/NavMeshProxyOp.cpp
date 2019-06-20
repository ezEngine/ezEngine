#include <EditorPluginRecastPCH.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpProxy_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOpProxy_BuildNavMesh::InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void ezLongOpProxy_BuildNavMesh::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpWorker_BuildNavMesh";

  {
    ezStringBuilder sComponentGuid, sOutputFile;
    ezConversionUtils::ToString(m_ComponentGuid, sComponentGuid);

    sOutputFile.Format(":project/AssetCache/Generated/{0}.ezRecastNavMesh", sComponentGuid);

    description << sOutputFile;
  }

  const ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(m_DocumentGuid);
  const ezDocumentObject* pObject = pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  ezVariant configGuid = pObject->GetTypeAccessor().GetValue("NavMeshConfig");

  const ezDocumentObject* pConfig = pDoc->GetObjectManager()->GetObject(configGuid.Get<ezUuid>());
  auto& cfg = pConfig->GetTypeAccessor();

  ezRecastConfig rcCfg;
  rcCfg.m_fAgentHeight = cfg.GetValue("AgentHeight").Get<float>();
  rcCfg.m_fAgentRadius = cfg.GetValue("AgentRadius").Get<float>();
  rcCfg.m_fAgentClimbHeight = cfg.GetValue("AgentClimbHeight").Get<float>();
  rcCfg.m_WalkableSlope = cfg.GetValue("WalkableSlope").Get<ezAngle>();
  rcCfg.m_fCellSize = cfg.GetValue("CellSize").Get<float>();
  rcCfg.m_fCellHeight = cfg.GetValue("CellHeight").Get<float>();
  rcCfg.m_fMinRegionSize = cfg.GetValue("MinRegionSize").Get<float>();
  rcCfg.m_fRegionMergeSize = cfg.GetValue("RegionMergeSize").Get<float>();
  rcCfg.m_fDetailMeshSampleDistanceFactor = cfg.GetValue("SampleDistanceFactor").Get<float>();
  rcCfg.m_fDetailMeshSampleErrorFactor = cfg.GetValue("SampleErrorFactor").Get<float>();
  rcCfg.m_fMaxSimplificationError = cfg.GetValue("MaxSimplification").Get<float>();
  rcCfg.m_fMaxEdgeLength = cfg.GetValue("MaxEdgeLength").Get<float>();
  rcCfg.Serialize(description);
}

void ezLongOpProxy_BuildNavMesh::Finalize(ezResult result, const ezDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    ezQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
