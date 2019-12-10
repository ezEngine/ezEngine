#include <EditorPluginScenePCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Baking/BakeSceneProxyOp.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_BakeScene, 1, ezRTTIDefaultAllocator<ezLongOpProxy_BakeScene>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOpProxy_BakeScene::InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void ezLongOpProxy_BakeScene::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpWorker_BakeScene";

  ezStringBuilder sOutputPath;
  sOutputPath.Format(":project/AssetCache/Generated/{0}", m_ComponentGuid);
  description << sOutputPath;
}

void ezLongOpProxy_BakeScene::Finalize(ezResult result, const ezDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    ezQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
