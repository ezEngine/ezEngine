#include <EnginePluginScenePCH.h>

#include <EnginePluginScene/Baking/BakeSceneWorkerOp.h>

#include <BakingPlugin/BakingScene.h>
#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Document/DocumentManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_BakeScene, 1, ezRTTIDefaultAllocator<ezLongOpWorker_BakeScene>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOpWorker_BakeScene::InitializeExecution(ezStreamReader& config, const ezUuid& DocumentGuid)
{
  ezEngineProcessDocumentContext* pDocContext = ezEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return EZ_FAILURE;

  config >> m_sOutputPath;

  {
    m_pScene = ezBakingScene::GetOrCreate(*pDocContext->GetWorld());

    EZ_SUCCEED_OR_RETURN(m_pScene->Extract());
  }

  return EZ_SUCCESS;
}

ezResult ezLongOpWorker_BakeScene::Execute(ezProgress& progress, ezStreamWriter& proxydata)
{
  EZ_SUCCEED_OR_RETURN(m_pScene->Bake(m_sOutputPath, progress));

#if 0
  ezProgressRange pgRange("Generating NavMesh", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  ezRecastNavMeshBuilder NavMeshBuilder;
  ezRecastNavMeshResourceDescriptor desc;

  if (!pgRange.BeginNextStep("Building NavMesh"))
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(NavMeshBuilder.Build(m_NavMeshConfig, m_ExtractedWorldGeometry, desc, progress));

  if (!pgRange.BeginNextStep("Writing Result"))
    return EZ_FAILURE;

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  ezAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  header.Write(file);

  EZ_SUCCEED_OR_RETURN(desc.Serialize(file));
#endif

  return EZ_SUCCESS;
}
