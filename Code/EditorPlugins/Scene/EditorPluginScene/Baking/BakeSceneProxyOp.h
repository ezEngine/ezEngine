#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class ezLongOpProxy_BakeScene : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_BakeScene, ezLongOpProxy);

public:
  virtual void InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Bake Scene"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;
  virtual void Finalize(ezResult result, const ezDataBuffer& resultData) override;

private:
  ezUuid m_DocumentGuid;
  ezUuid m_ComponentGuid;
};
