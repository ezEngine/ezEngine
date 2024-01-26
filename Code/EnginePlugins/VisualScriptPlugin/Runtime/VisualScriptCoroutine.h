#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptCoroutine : public ezScriptCoroutine
{
public:
  ezVisualScriptCoroutine(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc);
  ~ezVisualScriptCoroutine();

  virtual void StartWithVarargs(ezArrayPtr<ezVariant> arguments) override;
  virtual void Stop() override;
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) override;

private:
  ezVisualScriptDataStorage m_LocalDataStorage;
  ezVisualScriptExecutionContext m_Context;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptCoroutineAllocator : public ezRTTIAllocator
{
public:
  ezVisualScriptCoroutineAllocator(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc);

  void Deallocate(void* pObject, ezAllocator* pAllocator = nullptr) override;
  ezInternal::NewInstance<void> AllocateInternal(ezAllocator* pAllocator) override;

private:
  ezSharedPtr<const ezVisualScriptGraphDescription> m_pDesc;
};
