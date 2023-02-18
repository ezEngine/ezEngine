#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class ezLongOpWorker_BuildNavMesh : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_BuildNavMesh, ezLongOpWorker);

public:
  virtual ezResult InitializeExecution(ezStreamReader& ref_config, const ezUuid& documentGuid) override;
  virtual ezResult Execute(ezProgress& ref_progress, ezStreamWriter& ref_proxydata) override;

  ezString m_sOutputPath;
  ezRecastConfig m_NavMeshConfig;
  ezWorldGeoExtractionUtil::MeshObjectList m_ExtractedObjects;
};
