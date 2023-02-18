#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class ezBakingScene;

class ezLongOpWorker_BakeScene : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_BakeScene, ezLongOpWorker);

public:
  virtual ezResult InitializeExecution(ezStreamReader& ref_config, const ezUuid& documentGuid) override;
  virtual ezResult Execute(ezProgress& ref_progress, ezStreamWriter& ref_proxydata) override;

  ezString m_sOutputPath;
  ezBakingScene* m_pScene;
};
