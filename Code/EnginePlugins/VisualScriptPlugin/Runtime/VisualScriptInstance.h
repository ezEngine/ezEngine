#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptInstance : public ezScriptInstance
{
public:
  ezVisualScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, const ezSharedPtr<ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pInstanceDataDesc, const ezSharedPtr<ezVisualScriptInstanceDataMapping>& pInstanceDataMapping);

  virtual void SetInstanceVariable(const ezHashedString& sName, const ezVariant& value) override;
  virtual ezVariant GetInstanceVariable(const ezHashedString& sName) override;

  ezVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  ezVisualScriptDataStorage* GetInstanceDataStorage() { return &m_InstanceDataStorage; }

private:
  ezSharedPtr<ezVisualScriptDataStorage> m_pConstantDataStorage;
  ezSharedPtr<ezVisualScriptInstanceDataMapping> m_pInstanceDataMapping;

  ezVisualScriptDataStorage m_InstanceDataStorage;
};
