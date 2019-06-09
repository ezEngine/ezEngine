#include <EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpWorker_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOpWorker_BuildNavMesh::InitializeExecution(const ezUuid& DocumentGuid)
{
  ezEngineProcessDocumentContext* pDocContext = ezEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return EZ_FAILURE;

  pDocContext->GetWorld();

  ezRecastConfig config;

  EZ_LOCK(pDocContext->GetWorld()->GetWriteMarker());

  ezRecastNavMeshResourceDescriptor desc;

  ezRecastNavMeshBuilder NavMeshBuilder;
  EZ_SUCCEED_OR_RETURN(NavMeshBuilder.Build(config, *pDocContext->GetWorld(), desc));

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  ezAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);

  EZ_SUCCEED_OR_RETURN(desc.Serialize(file));

  return EZ_SUCCESS;
}

void ezLongOpWorker_BuildNavMesh::Execute(ezProgress& progress)
{
  ezProgressRange pgRange("Generating NavMesh", 100, true, &progress);

  for (ezUInt32 i = 0; i < 100; ++i)
  {
    pgRange.BeginNextStep("Step");
    ezThreadUtils::Sleep(ezTime::Milliseconds(50));
  }
}

void ezLongOpWorker_BuildNavMesh::InitializeReplicated(ezStreamReader& description)
{
  description >> m_sOutputPath;
}
