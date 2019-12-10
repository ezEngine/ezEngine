#include <EnginePluginScenePCH.h>

#include <EnginePluginScene/Baking/BakeSceneWorkerOp.h>

#include <BakingPlugin/BakingScene.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
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

  return EZ_SUCCESS;
}
