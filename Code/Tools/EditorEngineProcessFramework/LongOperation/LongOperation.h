#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezStringBuilder;
class ezStreamWriter;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperation : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperation, ezReflectedClass);

public:
  ezLongOperation() {}

  virtual const char* GetDisplayName() const = 0;
  virtual void Step() = 0;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) = 0;
  virtual void InitializeReplicated(ezStreamReader& description) = 0;

  ezUuid m_OperationGuid;
  ezUuid m_DocumentGuid;
  float m_fCompletation = 0.0f;
};


class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationLocal : public ezLongOperation
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal, ezLongOperation);

public:
};


class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationRemote : public ezLongOperation
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationRemote, ezLongOperation);

public:
  virtual const char* GetDisplayName() const override { return "Remote Operation"; }
  virtual void Step() override {}
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  virtual void InitializeReplicated(ezStreamReader& description) override {}
};


class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationLocal_Dummy : public ezLongOperationLocal
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal_Dummy, ezLongOperationLocal);

public:
  virtual const char* GetDisplayName() const override { return "Dummy Operation"; }
  virtual void Step() override;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;
  virtual void InitializeReplicated(ezStreamReader& description) override;

  ezTime m_Duration;
  ezTime m_TimeTaken;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationRemote_Dummy : public ezLongOperationRemote
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationRemote_Dummy, ezLongOperationRemote);

public:
  virtual void Step() override {}
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description) override;

  ezTime m_Duration;
};
