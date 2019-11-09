#pragma once

#include <BakingPlugin/Baking.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class ezLongOpWorker_BakeScene : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_BakeScene, ezLongOpWorker);

public:
  virtual ezResult InitializeExecution(ezStreamReader& config, const ezUuid& DocumentGuid) override;
  virtual ezResult Execute(ezProgress& progress, ezStreamWriter& proxydata) override;

  ezString m_sOutputPath;
  ezBaking::Scene m_Scene;
};
