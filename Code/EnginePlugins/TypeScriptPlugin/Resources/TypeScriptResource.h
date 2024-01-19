#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

class ezComponent;
class ezTypeScriptBinding;

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptInstance : public ezScriptInstance
{
public:
  ezTypeScriptInstance(ezComponent& inout_owner, ezWorld* pWorld, ezTypeScriptBinding& inout_binding);

  virtual void SetInstanceVariables(const ezArrayMap<ezHashedString, ezVariant>& parameters) override;
  virtual void SetInstanceVariable(const ezHashedString& sName, const ezVariant& value) override;
  virtual ezVariant GetInstanceVariable(const ezHashedString& sName) override;

  ezTypeScriptBinding& GetBinding() { return m_Binding; }

  ezComponent& GetComponent() { return static_cast<ezComponent&>(GetOwner()); }

private:
  ezTypeScriptBinding& m_Binding;
};

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptClassResource : public ezScriptClassResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptClassResource, ezScriptClassResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezTypeScriptClassResource);

public:
  ezTypeScriptClassResource();
  ~ezTypeScriptClassResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual ezUniquePtr<ezScriptInstance> Instantiate(ezReflectedClass& inout_owner, ezWorld* pWorld) const override;

private:
  ezUuid m_Guid;
};
