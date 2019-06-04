#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezStringBuilder;
class ezStreamWriter;
class ezTask;

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperation : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperation, ezReflectedClass);

public:
  ezLongOperation() = default;

  virtual const char* GetDisplayName() const = 0;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) = 0;
  virtual void InitializeReplicated(ezStreamReader& description) = 0;

  ezUuid m_OperationGuid;
  ezUuid m_DocumentGuid;
  float m_fCompletation = 0.0f;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationLocal : public ezLongOperation
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal, ezLongOperation);

public:
  /// \brief Sets up a default ezLongOperationRemoteReplicant. Typically does not need to be overridden by derived classes anymore.
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  virtual void Execute(const ezTask* pExecutingTask) = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationRemote : public ezLongOperation
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationRemote, ezLongOperation);

public:
  virtual void InitializeReplicated(ezStreamReader& description) override { EZ_ASSERT_NOT_IMPLEMENTED; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationRemoteReplicant final : public ezLongOperationRemote
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationRemoteReplicant, ezLongOperationRemote);

public:
  virtual const char* GetDisplayName() const override { return m_sDisplayName; }
  virtual void InitializeReplicated(ezStreamReader& description) override;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override { EZ_ASSERT_NOT_IMPLEMENTED; }

private:
  ezString m_sDisplayName;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationLocal_Dummy : public ezLongOperationLocal
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal_Dummy, ezLongOperationLocal);

public:
  virtual const char* GetDisplayName() const override { return "Dummy (Local)"; }
  virtual void Execute(const ezTask* pExecutingTask) override;
  virtual void InitializeReplicated(ezStreamReader& description) override;

  ezTime m_Duration;
  ezTime m_TimeTaken;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationRemote_Dummy : public ezLongOperationRemote
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationRemote_Dummy, ezLongOperationRemote);

public:
  virtual const char* GetDisplayName() const override { return "Dummy (Remote)"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  ezTime m_Duration;
};
