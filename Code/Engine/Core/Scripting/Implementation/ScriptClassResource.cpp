#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptClassResource, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezScriptClassResource);
// clang-format on

ezScriptClassResource::ezScriptClassResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezScriptClassResource::~ezScriptClassResource() = default;

ezSharedPtr<ezScriptRTTI> ezScriptClassResource::CreateScriptType(ezStringView sName, const ezRTTI* pBaseType, ezScriptRTTI::FunctionList&& functions, ezScriptRTTI::MessageHandlerList&& messageHandlers)
{
  ezScriptRTTI::FunctionList sortedFunctions;
  for (auto pFuncProp : pBaseType->GetFunctions())
  {
    auto pBaseClassFuncAttr = pFuncProp->GetAttributeByType<ezScriptBaseClassFunctionAttribute>();
    if (pBaseClassFuncAttr == nullptr)
      continue;

    ezStringView sBaseClassFuncName = pFuncProp->GetPropertyName();
    sBaseClassFuncName.TrimWordStart("Reflection_");

    ezUInt16 uiIndex = pBaseClassFuncAttr->GetIndex();
    sortedFunctions.EnsureCount(uiIndex + 1);

    for (ezUInt32 i = 0; i < functions.GetCount(); ++i)
    {
      auto& pScriptFuncProp = functions[i];
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        functions.RemoveAtAndSwap(i);
        break;
      }
    }
  }

  m_pType = EZ_SCRIPT_NEW(ezScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
  return m_pType;
}

void ezScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}

ezSharedPtr<ezScriptCoroutineRTTI> ezScriptClassResource::CreateScriptCoroutineType(ezStringView sScriptClassName, ezStringView sFunctionName, ezUniquePtr<ezRTTIAllocator>&& pAllocator)
{
  ezStringBuilder sCoroutineTypeName;
  sCoroutineTypeName.Set(sScriptClassName, "::", sFunctionName, "<Coroutine>");

  ezSharedPtr<ezScriptCoroutineRTTI> pCoroutineType = EZ_SCRIPT_NEW(ezScriptCoroutineRTTI, sCoroutineTypeName, std::move(pAllocator));
  m_CoroutineTypes.PushBack(pCoroutineType);

  return pCoroutineType;
}

void ezScriptClassResource::DeleteAllScriptCoroutineTypes()
{
  m_CoroutineTypes.Clear();
}


EZ_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptClassResource);
