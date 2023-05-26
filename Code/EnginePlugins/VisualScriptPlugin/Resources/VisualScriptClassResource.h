#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

struct ezVisualScriptDataDescription;
class ezVisualScriptDataStorage;

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptClassResource : public ezScriptClassResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptClassResource, ezScriptClassResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezVisualScriptClassResource);

public:
  ezVisualScriptClassResource();
  ~ezVisualScriptClassResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual ezUniquePtr<ezScriptInstance> Instantiate(ezReflectedClass& owner, ezWorld* pWorld) const override;

  ezSharedPtr<ezVisualScriptDataStorage> m_pConstantDataStorage;
  ezSharedPtr<const ezVisualScriptDataDescription> m_pInstanceDataDesc;
};
