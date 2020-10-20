#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezXRConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezXRConfig, ezProfileConfigData);

public:
  virtual void SaveRuntimeData(ezChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(ezChunkStreamReader& stream) override;

  bool m_bEnableXR = false;
  ezString m_sXRRenderPipeline;
};
