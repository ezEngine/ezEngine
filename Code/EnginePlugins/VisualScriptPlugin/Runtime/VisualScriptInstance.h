#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptInstance : public ezScriptInstance
{
public:
  ezVisualScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, const ezSharedPtr<ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pInstanceDataDesc);

  virtual void ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters) override;

  ezVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  ezVisualScriptDataStorage* GetInstanceDataStorage() { return m_pInstanceDataStorage.Borrow(); }

private:
  ezSharedPtr<ezVisualScriptDataStorage> m_pConstantDataStorage;
  ezUniquePtr<ezVisualScriptDataStorage> m_pInstanceDataStorage;
};

#include <VisualScriptPlugin/Runtime/VisualScriptInstance_inl.h>
