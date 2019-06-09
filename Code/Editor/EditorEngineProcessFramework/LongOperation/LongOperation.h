#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezStringBuilder;
class ezStreamWriter;
class ezTask;
class ezLongOpManager;
class ezProgress;

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOp : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOp, ezReflectedClass);

public:
  ezLongOp() = default;

  virtual const char* GetDisplayName() const = 0;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) = 0;
  virtual void InitializeReplicated(ezStreamReader& description) = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpWorker : public ezLongOp
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker, ezLongOp);

public:
  /// \brief Sets up a default ezLongOpProxyReplicant. Typically does not need to be overridden by derived classes anymore.
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  virtual ezResult InitializeExecution(const ezUuid& DocumentGuid) { return EZ_SUCCESS; }
  virtual ezResult Execute(ezProgress& progress) = 0;

private:
  friend ezLongOpManager;
  ezLongOpManager* m_pManager = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxy : public ezLongOp
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy, ezLongOp);

public:
  virtual void InitializeReplicated(ezStreamReader& description) override { EZ_ASSERT_NOT_IMPLEMENTED; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxy_Simple : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_Simple, ezLongOpProxy);

public:
  ezLongOpProxy_Simple(const char* szDisplayName, const char* szRecplicationOpType);

  virtual const char* GetDisplayName() const override;

  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

private:
  ezString m_sDisplayName;
  const char* m_szRecplicationOpType = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxyReplicant final : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxyReplicant, ezLongOpProxy);

public:
  virtual const char* GetDisplayName() const override { return m_sDisplayName; }
  virtual void InitializeReplicated(ezStreamReader& description) override;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

private:
  ezString m_sDisplayName;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpWorker_Dummy : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_Dummy, ezLongOpWorker);

public:
  virtual const char* GetDisplayName() const override { return "Dummy (Local)"; }
  virtual ezResult Execute(ezProgress& progress) override;
  virtual void InitializeReplicated(ezStreamReader& description) override;

  ezTime m_Duration;
  ezTime m_TimeTaken;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxy_Dummy : public ezLongOpProxy
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy_Dummy, ezLongOpProxy);

public:
  virtual const char* GetDisplayName() const override { return "Dummy (Remote)"; }
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  ezTime m_Duration;
};
