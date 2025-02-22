#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptClassResource.h>

class asIScriptModule;
class asITypeInfo;

using ezAngelScriptResourceHandle = ezTypedResourceHandle<class ezAngelScriptResource>;

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptResource : public ezScriptClassResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAngelScriptResource, ezScriptClassResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAngelScriptResource);

public:
  ezAngelScriptResource();
  ~ezAngelScriptResource();

  ezStringView GetScriptContent() const { return m_sScriptContent; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual ezUniquePtr<ezScriptInstance> Instantiate(ezReflectedClass& inout_owner, ezWorld* pWorld) const override;

  void FindMessageHandlers(const asITypeInfo* pClassType, ezScriptRTTI::MessageHandlerList& inout_Handlers);

  ezString m_sClassName;
  ezString m_sScriptContent;
  asIScriptModule* m_pModule = nullptr;
};
