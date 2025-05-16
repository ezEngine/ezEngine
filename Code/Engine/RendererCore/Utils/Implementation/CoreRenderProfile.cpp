#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Utils/CoreRenderProfile.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCoreRenderProfileConfig, 1, ezRTTIDefaultAllocator<ezCoreRenderProfileConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShadowAtlasTextureSize", m_uiShadowAtlasTextureSize)->AddAttributes(new ezDefaultValueAttribute(4096), new ezClampValueAttribute(512, 8192)),
    EZ_MEMBER_PROPERTY("MaxShadowMapSize", m_uiMaxShadowMapSize)->AddAttributes(new ezDefaultValueAttribute(1024), new ezClampValueAttribute(64, 4096)),
    EZ_MEMBER_PROPERTY("MinShadowMapSize", m_uiMinShadowMapSize)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(8, 512)),
    EZ_MEMBER_PROPERTY("RuntimeDecalAtlasTextureSize", m_uiRuntimeDecalAtlasTextureSize)->AddAttributes(new ezDefaultValueAttribute(3072), new ezClampValueAttribute(512, 8192)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezCoreRenderProfileConfig::SaveRuntimeData(ezChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("ezCoreRenderProfileConfig", 2);

  inout_stream << m_uiShadowAtlasTextureSize;
  inout_stream << m_uiMaxShadowMapSize;
  inout_stream << m_uiMinShadowMapSize;
  inout_stream << m_uiRuntimeDecalAtlasTextureSize;

  inout_stream.EndChunk();
}

void ezCoreRenderProfileConfig::LoadRuntimeData(ezChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "ezCoreRenderProfileConfig" && chunk.m_uiChunkVersion >= 1)
  {
    inout_stream >> m_uiShadowAtlasTextureSize;
    inout_stream >> m_uiMaxShadowMapSize;
    inout_stream >> m_uiMinShadowMapSize;

    if (chunk.m_uiChunkVersion >= 2)
    {
      inout_stream >> m_uiRuntimeDecalAtlasTextureSize;
    }
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_CoreRenderProfile);
