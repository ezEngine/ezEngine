#include <GameEnginePCH.h>

#include <GameEngine/Configuration/RendererProfileConfigs.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineProfileConfig, 1, ezRTTIDefaultAllocator<ezRenderPipelineProfileConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    // MainRenderPipeline.ezRenderPipelineAsset
    EZ_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"))),
    // EditorRenderPipeline.ezRenderPipelineAsset
    //EZ_MEMBER_PROPERTY("EditorRenderPipeline", m_sEditorRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"))),
    // DebugRenderPipeline.ezRenderPipelineAsset
    //EZ_MEMBER_PROPERTY("DebugRenderPipeline", m_sDebugRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"))),

    EZ_MAP_MEMBER_PROPERTY("CameraPipelines", m_CameraPipelines)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezRenderPipelineProfileConfig::SaveRuntimeData(ezChunkStreamWriter& stream) const
{
  stream.BeginChunk("ezRenderPipelineProfileConfig", 2);

  stream << m_sMainRenderPipeline;

  stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    stream << it.Key();
    stream << it.Value();
  }

  stream.EndChunk();
}

void ezRenderPipelineProfileConfig::LoadRuntimeData(ezChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "ezRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    ezRenderWorld::BeginModifyCameraConfigs();
    ezRenderWorld::ClearCameraConfigs();

    stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    ezUInt32 uiNumCamPipes = 0;
    stream >> uiNumCamPipes;
    for (ezUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      ezString sPipeName, sPipeAsset;

      stream >> sPipeName;
      stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;

      ezRenderWorld::CameraConfig cfg;
      cfg.m_hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(sPipeAsset);

      ezRenderWorld::SetCameraConfig(sPipeName, cfg);
    }

    ezRenderWorld::EndModifyCameraConfigs();
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_RendererProfileConfigs);

