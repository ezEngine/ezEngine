#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/RendererCoreDLL.h>

class EZ_RENDERERCORE_DLL ezCoreRenderProfileConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCoreRenderProfileConfig, ezProfileConfigData);

public:
  virtual void SaveRuntimeData(ezChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(ezChunkStreamReader& inout_stream) override;

  ezUInt32 m_uiShadowAtlasTextureSize = 4096;
  ezUInt32 m_uiMaxShadowMapSize = 1024;
  ezUInt32 m_uiMinShadowMapSize = 64;

  ezUInt32 m_uiRuntimeDecalAtlasTextureSize = 3072;
};
