#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

/// Editor side of the automatic reflection probe placement.
///
/// Reads the settings from the ezReflectionProbePlacementComponent it is attached to, has the engine
/// process compute the probe positions, and then creates the probe objects in the document.
class ezLongOpProxy_PlaceReflectionProbes : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_PlaceReflectionProbes, ezLongOpProxy);

public:
  virtual void InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Place Reflection Probes"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& inout_config) override;
  virtual void Finalize(ezResult result, const ezDataBuffer& resultData) override;

private:
  ezUuid m_DocumentGuid;
  ezUuid m_ComponentGuid;
};
