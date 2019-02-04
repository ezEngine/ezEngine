#pragma once

#include <GameEngine/Configuration/PlatformProfile.h>

class EZ_GAMEENGINE_DLL ezRenderPipelineProfileConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineProfileConfig, ezProfileConfigData);

public:
  virtual void SaveRuntimeData(ezChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(ezChunkStreamReader& stream) override;

  ezString m_sMainRenderPipeline;
  // ezString m_sEditorRenderPipeline;
  // ezString m_sDebugRenderPipeline;

  ezMap<ezString, ezString> m_CameraPipelines;
};

