#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptRTTI.h>

class ezWorld;
using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;

/// \brief Resource representing a script class with its type information and instantiation capabilities.
///
/// Base class for script resources that define class types for scripting languages. Manages
/// script type creation, instantiation, and coroutine type handling. Derived classes implement
/// language-specific instantiation logic.
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
  ezDynamicArray<ezSharedPtr<ezScriptCoroutineRTTI>> m_CoroutineTypes;
};
