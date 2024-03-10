#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpWorker_BuildNavMesh>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOpWorker_BuildNavMesh::InitializeExecution(ezStreamReader& ref_config, const ezUuid& documentGuid)
{
  ezEngineProcessDocumentContext* pDocContext = ezEngineProcessDocumentContext::GetDocumentContext(documentGuid);

  if (pDocContext == nullptr)
    return EZ_FAILURE;

  ref_config >> m_sOutputPath;
  EZ_SUCCEED_OR_RETURN(m_NavMeshConfig.Deserialize(ref_config));

  EZ_SUCCEED_OR_RETURN(ezRecastNavMeshBuilder::ExtractWorldGeometry(*pDocContext->GetWorld(), m_ExtractedObjects));

  return EZ_SUCCESS;
}

ezResult ezLongOpWorker_BuildNavMesh::Execute(ezProgress& ref_progress, ezStreamWriter& ref_proxydata)
{
  ezProgressRange pgRange("Generating NavMesh", 2, true, &ref_progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  ezRecastNavMeshBuilder NavMeshBuilder;
  ezRecastNavMeshResourceDescriptor desc;

  if (!pgRange.BeginNextStep("Building NavMesh"))
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(NavMeshBuilder.Build(m_NavMeshConfig, m_ExtractedObjects, desc, ref_progress));

  if (!pgRange.BeginNextStep("Writing Result"))
    return EZ_FAILURE;

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  ezAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  EZ_SUCCEED_OR_RETURN(header.Write(file));

  EZ_SUCCEED_OR_RETURN(desc.Serialize(file));

  return EZ_SUCCESS;
}
