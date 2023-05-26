#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>

ezScriptRTTI::ezScriptRTTI(ezStringView sName, const ezRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : ezRTTI(nullptr, pParentType, 0, 1, ezVariantType::Invalid, ezTypeFlags::Class, nullptr, ezArrayPtr<ezAbstractProperty*>(), ezArrayPtr<ezAbstractFunctionProperty*>(), ezArrayPtr<ezPropertyAttribute*>(), ezArrayPtr<ezAbstractMessageHandler*>(), ezArrayPtr<ezMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_FunctionStorage(std::move(functions))
  , m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_sTypeName = m_sTypeNameStorage.GetData();

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
  m_sTypeName = nullptr;
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

ezScriptFunctionProperty::ezScriptFunctionProperty(ezStringView sName)
  : ezAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage.Assign(sName);
  m_szPropertyName = m_sPropertyNameStorage.GetData();
}

ezScriptFunctionProperty::~ezScriptFunctionProperty() = default;

//////////////////////////////////////////////////////////////////////////

ezScriptInstance::ezScriptInstance(ezReflectedClass& ref_owner, ezWorld* pWorld)
  : m_Owner(ref_owner)
  , m_pWorld(pWorld)
{
}
