#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezStringBuilder;
class ezStreamWriter;
class ezTask;
class ezLongOpManager;
class ezProgress;

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpWorker : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker, ezReflectedClass);

public:
  virtual ezResult InitializeExecution(ezStreamReader& config, const ezUuid& DocumentGuid) { return EZ_SUCCESS; }
  virtual ezResult Execute(ezProgress& progress, ezStreamWriter& proxydata) = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxy : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy, ezReflectedClass);

public:
  virtual const char* GetDisplayName() const = 0;
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& config) = 0;
  virtual void InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid) {}
  virtual void Finalize(ezResult result, const ezDataBuffer& resultData) {}
};

