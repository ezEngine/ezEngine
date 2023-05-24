#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

ezScriptRTTI::ezScriptRTTI(ezStringView sName, const ezRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : ezRTTI(nullptr, pParentType, 0, 1, ezVariantType::Invalid, ezTypeFlags::Class, nullptr, ezArrayPtr<ezAbstractProperty*>(), ezArrayPtr<ezAbstractFunctionProperty*>(), ezArrayPtr<ezPropertyAttribute*>(), ezArrayPtr<ezAbstractMessageHandler*>(), ezArrayPtr<ezMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_FunctionStorage(std::move(functions))
  , m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_szTypeName = m_sTypeNameStorage.GetData();

  for (auto& pFunction : m_FunctionStorage)
  {
    if (pFunction != nullptr)
    {
      m_FunctionRawPtrs.PushBack(pFunction.Borrow());
    }
  }

  for (auto& pMessageHandler : m_MessageHandlerStorage)
  {
    if (pMessageHandler != nullptr)
    {
      m_MessageHandlerRawPtrs.PushBack(pMessageHandler.Borrow());
    }
  }

  m_Functions = m_FunctionRawPtrs;
  m_MessageHandlers = m_MessageHandlerRawPtrs;

  RegisterType();

  SetupParentHierarchy();
  GatherDynamicMessageHandlers();
}

ezScriptRTTI::~ezScriptRTTI()
{
  UnregisterType();
  m_szTypeName = nullptr;
}

const ezAbstractFunctionProperty* ezScriptRTTI::GetFunctionByIndex(ezUInt32 uiIndex) const
{
  if (uiIndex < m_FunctionStorage.GetCount())
  {
    return m_FunctionStorage.GetData()[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

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

void ezScriptClassResource::CreateScriptType(ezStringView sName, const ezRTTI* pBaseType, ezScriptRTTI::FunctionList&& functions, ezScriptRTTI::MessageHandlerList&& messageHandlers)
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

    for (auto& pScriptFuncProp : functions)
    {
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        break;
      }
    }
  }

  m_pType = EZ_DEFAULT_NEW(ezScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
}

void ezScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}
