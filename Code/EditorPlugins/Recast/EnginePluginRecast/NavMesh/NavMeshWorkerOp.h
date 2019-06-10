#include <EnginePluginRecastPCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

class ezLongOpWorker_BuildNavMesh : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_BuildNavMesh, ezLongOpWorker);

public:
  virtual ezResult InitializeExecution(ezStreamReader& config, const ezUuid& DocumentGuid) override;
  virtual ezResult Execute(ezProgress& progress, ezStreamWriter& proxydata) override;

  ezString m_sOutputPath;
  ezRecastConfig m_NavMeshConfig;
  ezWorldGeoExtractionUtil::Geometry m_ExtractedWorldGeometry;
};
