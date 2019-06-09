#include <EditorPluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>

class ezLongOpProxy_BuildNavMesh : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_BuildNavMesh, ezLongOpProxy);

public:
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  ezString m_sOutputPath;
};
