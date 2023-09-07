#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptBasicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

namespace
{
  const void* GetDataPointer(const ezVariant& var, const ezRTTI* pTargetType)
  {
    if (pTargetType == ezGetStaticRTTI<ezVariant>())
    {
      return &var;
    }

    return var.GetData();
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Sequence, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Sequence>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then1", 0),
    EZ_OUTPUT_EXECUTION_PIN("then2", 1),
    EZ_OUTPUT_EXECUTION_PIN("then3", 2),
    EZ_OUTPUT_EXECUTION_PIN("then4", 3),
    EZ_OUTPUT_EXECUTION_PIN("then5", 4),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Sequence::ezVisualScriptNode_Sequence() = default;
ezVisualScriptNode_Sequence::~ezVisualScriptNode_Sequence() = default;

void ezVisualScriptNode_Sequence::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
  pInstance->ExecuteConnectedNodes(this, 1);
  pInstance->ExecuteConnectedNodes(this, 2);
  pInstance->ExecuteConnectedNodes(this, 3);
  pInstance->ExecuteConnectedNodes(this, 4);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Delay, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Delay>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("Delay: '{Delay}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Delay", 0, ezVisualScriptDataPinType::Number, m_Delay),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Delay::ezVisualScriptNode_Delay()
{
  ezStringBuilder sb;
  sb.Format("VisualSciptDelay_{}", ezArgP(this));

  m_sMessage.Assign(sb.GetView());
}

ezVisualScriptNode_Delay::~ezVisualScriptNode_Delay() = default;

void ezVisualScriptNode_Delay::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bMessageReceived)
  {
    pInstance->ExecuteConnectedNodes(this, 0);
    m_bMessageReceived = false;
  }
  else
  {
    ezMsgComponentInternalTrigger msg;
    msg.m_sMessage = m_sMessage;
    pInstance->GetWorld()->PostMessage(pInstance->GetOwnerComponent(), msg, m_Delay);
  }
}

void* ezVisualScriptNode_Delay::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Delay;
  }

  return nullptr;
}

ezInt32 ezVisualScriptNode_Delay::HandlesMessagesWithID() const
{
  return ezMsgComponentInternalTrigger::GetTypeMsgId();
}

void ezVisualScriptNode_Delay::HandleMessage(ezMessage* pMsg)
{
  ezMsgComponentInternalTrigger& msg = *static_cast<ezMsgComponentInternalTrigger*>(pMsg);

  if (msg.m_sMessage == m_sMessage)
  {
    m_bMessageReceived = true;
    m_bStepNode = true;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Log, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Log>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Debug"),
    new ezTitleAttribute("Log: '{Text}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Properties
    EZ_MEMBER_PROPERTY("Text", m_sLog)->AddAttributes(new ezDefaultValueAttribute(ezStringView("Value1: {0}, Value2: {1}, Value3: {2}"))),
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_MEMBER_PROPERTY("Value0", m_Value0)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new ezVisScriptDataPinInAttribute(1, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new ezVisScriptDataPinInAttribute(2, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Log::ezVisualScriptNode_Log() = default;
ezVisualScriptNode_Log::~ezVisualScriptNode_Log() = default;

void ezVisualScriptNode_Log::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezLog::Info(m_sLog, m_Value0, m_Value1, m_Value2);

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_Log::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value0;
    case 1:
      return &m_Value1;
    case 2:
      return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_MessageSender, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_MessageSender>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Messages"),
    new ezHiddenAttribute()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_MessageSender::ezVisualScriptNode_MessageSender() = default;
ezVisualScriptNode_MessageSender::~ezVisualScriptNode_MessageSender()
{
  for (ezUInt32 uiProp = 0; uiProp < m_PropertyIndexToDataPinType.GetCount(); ++uiProp)
  {
    auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
    if (dataPinType == ezVisualScriptDataPinType::None)
      continue;

    ezUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    void* ptr = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

    switch (dataPinType)
    {
      case ezVisualScriptDataPinType::String:
        static_cast<ezString*>(ptr)->~ezString();
        break;

      case ezVisualScriptDataPinType::Variant:
        static_cast<ezVariant*>(ptr)->~ezVariant();
        break;

      default:
        // Nothing to do for other types
        break;
    }
  }

  m_ScratchMemory.Clear();
}

void ezVisualScriptNode_MessageSender::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_pMessageToSend != nullptr)
  {
    // fill message properties from inputs
    ezHybridArray<const ezAbstractProperty*, 32> properties;
    m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

    const ezUInt8 uiPropCount = static_cast<ezUInt8>(properties.GetCount());
    for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
    {
      auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
      if (dataPinType == ezVisualScriptDataPinType::None)
        continue;

      ezUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
      void* ptr = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

      ezVariant var;

      switch (dataPinType)
      {
        case ezVisualScriptDataPinType::Number:
          var = *static_cast<double*>(ptr);
          break;

        case ezVisualScriptDataPinType::Boolean:
          var = *static_cast<bool*>(ptr);
          break;

        case ezVisualScriptDataPinType::Vec3:
          var = *static_cast<ezVec3*>(ptr);
          break;

        case ezVisualScriptDataPinType::String:
          var = *static_cast<ezString*>(ptr);
          break;

        case ezVisualScriptDataPinType::Variant:
          var = *static_cast<ezVariant*>(ptr);
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      auto pAbsMember = static_cast<const ezAbstractMemberProperty*>(properties[uiProp]);
      ezReflectionUtils::SetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow(), var);
    }


    ezWorld* pWorld = pInstance->GetWorld();

    if (m_Delay.GetSeconds() == 0)
    {
      // Delay == 0 -> SendMessage

      if (!m_hComponent.IsInvalidated())
      {
        ezComponent* pComponent = nullptr;
        if (pWorld->TryGetComponent(m_hComponent, pComponent))
        {
          pComponent->SendMessage(*m_pMessageToSend);
        }
      }
      else
      {
        ezGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;
        ezGameObject* pObject = nullptr;
        if (pWorld->TryGetObject(hObject, pObject))
        {
          if (m_bRecursive)
          {
            pObject->SendMessageRecursive(*m_pMessageToSend);
          }
          else
          {
            pObject->SendMessage(*m_pMessageToSend);
          }
        }
      }

      {
        // could skip this, if we knew that there are no output pins, at all
        for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
        {
          if (properties[uiProp]->GetCategory() == ezPropertyCategory::Member &&
              properties[uiProp]->GetFlags().IsAnySet(ezPropertyFlags::VarInOut | ezPropertyFlags::VarOut))
          {
            auto pAbsMember = static_cast<const ezAbstractMemberProperty*>(properties[uiProp]);

            const ezRTTI* pType = pAbsMember->GetSpecificType();
            if (ezVisualScriptDataPinType::IsTypeSupported(pType))
            {
              ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow());
              ezVisualScriptDataPinType::EnforceSupportedType(var);
              pInstance->SetOutputPinValue(this, uiProp, GetDataPointer(var, pType));
            }
          }
        }
      }
    }
    else
    {
      // Delay > 0 -> PostMessage

      if (!m_hComponent.IsInvalidated())
      {
        pWorld->PostMessage(m_hComponent, *m_pMessageToSend, m_Delay);
      }
      else
      {
        ezGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;
        if (m_bRecursive)
        {
          pWorld->PostMessageRecursive(hObject, *m_pMessageToSend, m_Delay);
        }
        else
        {
          pWorld->PostMessage(hObject, *m_pMessageToSend, m_Delay);
        }
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_MessageSender::GetInputPinDataPointer(ezUInt8 uiPin)
{
  if (uiPin == 0)
    return &m_hObject;

  if (uiPin == 1)
    return &m_hComponent;

  if (uiPin == 2)
    return &m_Delay;

  if (m_pMessageToSend != nullptr)
  {
    const ezUInt32 uiProp = uiPin - 3;

    ezUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    if (uiOffset != 0xFFFF)
    {
      void* pPropertyPointer = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];
      return pPropertyPointer;
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return nullptr;
}

void ezVisualScriptNode_MessageSender::SetMessageToSend(ezUniquePtr<ezMessage>&& pMsg)
{
  m_pMessageToSend = std::move(pMsg);

  // Calculate scratch memory size and build mapping from property index to memory offset
  ezUInt32 uiScratchMemorySize = 0;

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

  const ezUInt8 uiPropCount = static_cast<ezUInt8>(properties.GetCount());
  m_PropertyIndexToMemoryOffset.SetCount(uiPropCount, 0xFFFF);
  m_PropertyIndexToDataPinType.SetCount(uiPropCount);

  for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    if (properties[uiProp]->GetCategory() == ezPropertyCategory::Member)
    {
      auto pAbsMember = static_cast<const ezAbstractMemberProperty*>(properties[uiProp]);

      const ezRTTI* pType = pAbsMember->GetSpecificType();
      auto dataPinType = ezVisualScriptDataPinType::GetDataPinTypeForType(pType);
      if (dataPinType == ezVisualScriptDataPinType::None)
        continue;

      m_PropertyIndexToMemoryOffset[uiProp] = static_cast<ezUInt16>(uiScratchMemorySize);
      m_PropertyIndexToDataPinType[uiProp] = dataPinType;

      uiScratchMemorySize += ezMath::Max<ezUInt32>(ezVisualScriptDataPinType::GetStorageByteSize(dataPinType), EZ_ALIGNMENT_MINIMUM);
    }
  }

  m_ScratchMemory.SetCountUninitialized(uiScratchMemorySize);
  m_ScratchMemory.ZeroFill();

  // Default construct and assign initial values
  for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
    if (dataPinType == ezVisualScriptDataPinType::None)
      continue;

    ezUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    void* ptr = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

    auto pAbsMember = static_cast<const ezAbstractMemberProperty*>(properties[uiProp]);
    ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow());
    ezVisualScriptDataPinType::EnforceSupportedType(var);

    switch (dataPinType)
    {
      case ezVisualScriptDataPinType::String:
        new (ptr) ezString(var.Get<ezString>());
        break;

      case ezVisualScriptDataPinType::Variant:
        new (ptr) ezVariant(var);
        break;

      default:
        ezMemoryUtils::RawByteCopy(ptr, var.GetData(), ezVisualScriptDataPinType::GetStorageByteSize(dataPinType));
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_MessageHandler, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_MessageHandler>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Messages"),
    new ezHiddenAttribute()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_MessageHandler::ezVisualScriptNode_MessageHandler() = default;
ezVisualScriptNode_MessageHandler::~ezVisualScriptNode_MessageHandler() = default;

void ezVisualScriptNode_MessageHandler::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_pMsgCopy == nullptr)
    return;

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  m_pMsgCopy->GetDynamicRTTI()->GetAllProperties(properties);

  const ezUInt8 uiPropCount = static_cast<ezUInt8>(properties.GetCount());
  for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    auto prop = properties[uiProp];

    if (prop->GetCategory() == ezPropertyCategory::Member)
    {
      auto pAbsMember = static_cast<const ezAbstractMemberProperty*>(prop);

      const ezRTTI* pType = pAbsMember->GetSpecificType();
      if (ezVisualScriptDataPinType::IsTypeSupported(pType))
      {
        ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMsgCopy.Borrow());
        ezVisualScriptDataPinType::EnforceSupportedType(var);
        pInstance->SetOutputPinValue(this, uiProp, GetDataPointer(var, pType));
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);

  m_pMsgCopy = nullptr;
}

ezInt32 ezVisualScriptNode_MessageHandler::HandlesMessagesWithID() const
{
  ezInt32 res = -1;

  if (m_pMessageTypeToHandle != nullptr && m_pMessageTypeToHandle->IsDerivedFrom<ezMessage>() && m_pMessageTypeToHandle->GetAllocator()->CanAllocate())
  {
    ezUniquePtr<ezMessage> pMsg = m_pMessageTypeToHandle->GetAllocator()->Allocate<ezMessage>();
    res = pMsg->GetId();
  }

  return res;
}

void ezVisualScriptNode_MessageHandler::HandleMessage(ezMessage* pMsg)
{
  ezRTTIAllocator* pMsgRTTIAllocator = pMsg->GetDynamicRTTI()->GetAllocator();
  m_pMsgCopy = pMsgRTTIAllocator->Clone<ezMessage>(pMsg);

  m_bStepNode = true;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FunctionCall, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FunctionCall>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Functions"),
    new ezHiddenAttribute()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_FunctionCall::ezVisualScriptNode_FunctionCall() = default;

ezVisualScriptNode_FunctionCall::~ezVisualScriptNode_FunctionCall() = default;

void ezVisualScriptNode_FunctionCall::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_pFunctionToCall == nullptr)
    return;

  ezWorld* pWorld = pInstance->GetWorld();

  if (m_hComponent.IsInvalidated())
  {
    // no component given -> try to look up the component type on the object instead

    if (m_hObject.IsInvalidated())
    {
      ezLog::Error("VisScript function call: Target component or object is not specified");

      m_pFunctionToCall = nullptr;
      return;
    }

    ezGameObject* pObject;
    if (!pWorld->TryGetObject(m_hObject, pObject))
    {
      // object is dead -> deactivate this node silently
      m_pFunctionToCall = nullptr;
      return;
    }

    ezComponent* pComponent;
    if (!pObject->TryGetComponentOfBaseType(m_pExpectedType, pComponent))
    {
      ezLog::Error("VisScript function call: Target object does not have a component of type {}", m_pExpectedType->GetTypeName());

      m_pFunctionToCall = nullptr;
      return;
    }

    m_hComponent = pComponent->GetHandle();
  }

  ezComponent* pComponent = nullptr;
  if (!pWorld->TryGetComponent(m_hComponent, pComponent))
  {
    // component is dead -> deactivate this node silently
    m_pFunctionToCall = nullptr;
    return;
  }

  if (!pComponent->GetDynamicRTTI()->IsDerivedFrom(m_pExpectedType))
  {
    ezLog::Error("VisScript function call: Target component of type '{}' is not of the expected base type '{}'",
      pComponent->GetDynamicRTTI()->GetTypeName(), m_pExpectedType->GetTypeName());

    m_pFunctionToCall = nullptr;
    return;
  }

  for (ezUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    const ezRTTI* pArgumentType = m_pFunctionToCall->GetArgumentType(arg);
    if (pArgumentType == ezGetStaticRTTI<ezVariant>())
      continue; // Nothing to do

    const ezVariant& var = m_Arguments[arg];
    const ezVariantType::Enum targetType = pArgumentType->GetVariantType();

    if (ConvertArgumentToRequiredType(m_Arguments[arg], targetType).Failed())
    {
      ezLog::Error("VisScript function call: Could not convert argument {} from variant type '{}' to target type '{}'", arg, (int)var.GetType(),
        (int)targetType);

      // probably a stale script with a mismatching pin <-> argument configuration
      m_pFunctionToCall = nullptr;
      return;
    }
  }

  // call the function on the target object
  m_pFunctionToCall->Execute(pComponent, m_Arguments, m_ReturnValue);

  // now we need to pull the data from return values and out parameters and pass them into our output pins
  ezUInt8 uiOutputPinIndex = 0;

  if (m_ReturnValue.IsValid())
  {
    ezVisualScriptDataPinType::EnforceSupportedType(m_ReturnValue);
    pInstance->SetOutputPinValue(this, uiOutputPinIndex, GetDataPointer(m_ReturnValue, m_pFunctionToCall->GetReturnType()));
    ++uiOutputPinIndex;
  }

  for (ezUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    // also do this for non-out parameters, as an 'in' parameter may still be a non-const reference (bad but valid)
    ezVisualScriptDataPinType::EnforceSupportedType(m_Arguments[arg]);

    if ((m_ArgumentIsOutParamMask & EZ_BIT(arg)) != 0) // if this argument represents an out or inout parameter, pull the data
    {
      const ezRTTI* pArgumentType = m_pFunctionToCall->GetArgumentType(arg);

      pInstance->SetOutputPinValue(this, uiOutputPinIndex, GetDataPointer(m_Arguments[arg], pArgumentType));
      ++uiOutputPinIndex;
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_FunctionCall::GetInputPinDataPointer(ezUInt8 uiPin)
{
  if (uiPin == 0)
    return &m_hObject;

  if (uiPin == 1)
    return &m_hComponent;

  if (uiPin >= m_Arguments.GetCount() + 2)
    return &m_ReturnValue; // unused dummy just to return anything in case of a mismatch

  return m_Arguments[uiPin - 2].GetWriteAccess().m_pObject;
}

ezResult ezVisualScriptNode_FunctionCall::ConvertArgumentToRequiredType(ezVariant& ref_var, ezVariantType::Enum type)
{
  if (ref_var.GetType() == type)
    return EZ_SUCCESS;

  ezResult couldConvert = EZ_FAILURE;
  ref_var = ref_var.ConvertTo(type, &couldConvert);

  return couldConvert;
}

//////////////////////////////////////////////////////////////////////////


EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptBasicNodes);
