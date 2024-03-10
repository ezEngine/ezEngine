#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Configuration/XRConfig.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezXRConfig, 2, ezRTTIDefaultAllocator<ezXRConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EnableXR", m_bEnableXR),
    // HololensRenderPipeline.ezRenderPipelineAsset
    EZ_MEMBER_PROPERTY("XRRenderPipeline", m_sXRRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezXRConfig::SaveRuntimeData(ezChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("ezXRConfig", 2);

  inout_stream << m_bEnableXR;
  inout_stream << m_sXRRenderPipeline;

  inout_stream.EndChunk();
}

void ezXRConfig::LoadRuntimeData(ezChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "ezVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "ezXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezVRConfig_1_2 : public ezGraphPatch
{
public:
  ezVRConfig_1_2()
    : ezGraphPatch("ezVRConfig", 5)
  {
  }
  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

ezVRConfig_1_2 g_ezVRConfig_1_2;

EZ_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
