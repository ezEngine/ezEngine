#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

#ifdef BUILDSYSTEM_ENABLE_RECAST_SUPPORT

#  include <AiPlugin/Utils/ReflectionProbePlacement.h>

/// Analyzes the scene geometry in the engine process and computes reflection probe positions.
///
/// The result is sent back to ezLongOpProxy_PlaceReflectionProbes, which creates the actual objects.
class ezLongOpWorker_PlaceReflectionProbes : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_PlaceReflectionProbes, ezLongOpWorker);

public:
  virtual ezResult InitializeExecution(ezStreamReader& ref_config, const ezUuid& documentGuid) override;
  virtual ezResult Execute(ezProgress& ref_progress, ezStreamWriter& ref_proxydata) override;

private:
  ezReflectionProbePlacementSettings m_Settings;
  ezReflectionProbePlacer m_Placer;
};

#endif
