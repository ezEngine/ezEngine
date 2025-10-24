#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/RendererCoreDLL.h>

/// Platform profile configuration for core rendering settings.
///
/// Allows different rendering quality settings per platform (PC, console, mobile, etc.).
/// Settings are stored in platform profile files and can be overridden at runtime.
class EZ_RENDERERCORE_DLL ezCoreRenderProfileConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCoreRenderProfileConfig, ezProfileConfigData);

public:
  virtual void SaveRuntimeData(ezChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(ezChunkStreamReader& inout_stream) override;

  ezUInt32 m_uiShadowAtlasTextureSize = 4096;       ///< Size of the texture atlas used for shadow maps.
  ezUInt32 m_uiMaxShadowMapSize = 1024;             ///< Maximum size for individual shadow maps.
  ezUInt32 m_uiMinShadowMapSize = 64;               ///< Minimum size for individual shadow maps.

  ezUInt32 m_uiRuntimeDecalAtlasTextureSize = 3072; ///< Size of the texture atlas for runtime decals.
};
