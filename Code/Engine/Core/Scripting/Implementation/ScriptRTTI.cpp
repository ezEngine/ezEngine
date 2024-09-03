#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/ReflectionUtils.h>

ezScriptRTTI::ezScriptRTTI(ezStringView sName, const ezRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : ezRTTI(nullptr, pParentType, 0, 1, ezVariantType::Invalid, ezTypeFlags::Class, nullptr, ezArrayPtr<const ezAbstractProperty*>(), ezArrayPtr<const ezAbstractFunctionProperty*>(), ezArrayPtr<const ezPropertyAttribute*>(), ezArrayPtr<ezAbstractMessageHandler*>(), ezArrayPtr<ezMessageSenderInfo>(), nullptr)
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

ezScriptMessageHandler::ezScriptMessageHandler(const ezScriptMessageDesc& desc)
  : m_Properties(desc.m_Properties)
{
  ezUniquePtr<ezMessage> pMessage = desc.m_pType->GetAllocator()->Allocate<ezMessage>();

  m_Id = pMessage->GetId();
  m_bIsConst = true;
}

ezScriptMessageHandler::~ezScriptMessageHandler() = default;

void ezScriptMessageHandler::FillMessagePropertyValues(const ezMessage& msg, ezDynamicArray<ezVariant>& out_propertyValues)
{
  out_propertyValues.Clear();

  for (auto pProp : m_Properties)
  {
    if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      out_propertyValues.PushBack(ezReflectionUtils::GetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), &msg));
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezScriptInstance::ezScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld)
  : m_Owner(inout_owner)
  , m_pWorld(pWorld)
{
}

void ezScriptInstance::SetInstanceVariables(const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
  for (auto it : parameters)
  {
    SetInstanceVariable(it.key, it.value);
  }
}

//////////////////////////////////////////////////////////////////////////

// static
ezAllocator* ezScriptAllocator::GetAllocator()
{
  static ezProxyAllocator s_ScriptAllocator("Script", ezFoundation::GetDefaultAllocator());
  return &s_ScriptAllocator;
}


