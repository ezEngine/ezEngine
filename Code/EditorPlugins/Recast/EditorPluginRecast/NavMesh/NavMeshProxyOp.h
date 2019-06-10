#include <EditorPluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class ezLongOpProxy_BuildNavMesh : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_BuildNavMesh, ezLongOpProxy);

public:
  virtual void InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;
  virtual void Finalize(ezResult result, const ezDataBuffer& resultData) override;

private:
  ezUuid m_DocumentGuid;
  ezUuid m_ComponentGuid;
};
