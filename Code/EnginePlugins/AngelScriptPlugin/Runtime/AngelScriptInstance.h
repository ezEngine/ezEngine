#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Containers/Blob.h>

class asIScriptContext;
class asIScriptModule;
class asIScriptObject;
class ezComponent;
class ezScriptComponent;

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptInstance : public ezScriptInstance
{
public:
  ezAngelScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, asIScriptModule* pModule, const char* szObjectTypeName);
  ~ezAngelScriptInstance();

  virtual void SetInstanceVariable(const ezHashedString& sName, const ezVariant& value) override;
  virtual ezVariant GetInstanceVariable(const ezHashedString& sName) override;

  asIScriptContext* GetContext() const { return m_pContext; }
  asIScriptObject* GetObject() const { return m_pObject; }
  ezScriptComponent* GetOwnerComponent() const { return m_pOwnerComponent; }

private:
  asIScriptObject* m_pObject = nullptr;
  asIScriptContext* m_pContext = nullptr;
  ezScriptComponent* m_pOwnerComponent = nullptr;
};
