#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptRTTI.h>

class ezWorld;
using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;

class EZ_CORE_DLL ezScriptClassResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptClassResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezScriptClassResource);

public:
  ezScriptClassResource();
  ~ezScriptClassResource();

  const ezSharedPtr<ezScriptRTTI>& GetType() const { return m_pType; }

  virtual ezUniquePtr<ezScriptInstance> Instantiate(ezReflectedClass& inout_owner, ezWorld* pWorld) const = 0;

protected:
  ezSharedPtr<ezScriptRTTI> CreateScriptType(ezStringView sName, const ezRTTI* pBaseType, ezScriptRTTI::FunctionList&& functions, ezScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  ezSharedPtr<ezScriptCoroutineRTTI> CreateScriptCoroutineType(ezStringView sScriptClassName, ezStringView sFunctionName, ezUniquePtr<ezRTTIAllocator>&& pAllocator);
  void DeleteAllScriptCoroutineTypes();

  ezSharedPtr<ezScriptRTTI> m_pType;
  ezDynamicArray<ezSharedPtr<ezScriptCoroutineRTTI>> m_pCoroutineTypes;
};
