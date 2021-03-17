#include <GameEnginePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptDataPinType, 1)
  EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::None, ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Vec3, ezVisualScriptDataPinType::String)
  EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptDataPinType::Variant)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezVisualScriptDataPinType::Enum ezVisualScriptDataPinType::GetDataPinTypeForType(const ezRTTI* pType)
{
  auto varType = pType->GetVariantType();
  if (varType >= ezVariant::Type::Int8 && varType <= ezVariant::Type::Double)
  {
    return ezVisualScriptDataPinType::Number;
  }

  switch (varType)
  {
    case ezVariantType::Bool:
      return ezVisualScriptDataPinType::Boolean;

    case ezVariantType::Vector3:
      return ezVisualScriptDataPinType::Vec3;

    case ezVariantType::String:
      return ezVisualScriptDataPinType::String;

    default:
      return pType == ezGetStaticRTTI<ezVariant>() ? ezVisualScriptDataPinType::Variant : ezVisualScriptDataPinType::None;
  }
}

// static
void ezVisualScriptDataPinType::EnforceSupportedType(ezVariant& var)
{
  switch (var.GetType())
  {
    case ezVariantType::Int8:
    case ezVariantType::UInt8:
    case ezVariantType::Int16:
    case ezVariantType::UInt16:
    case ezVariantType::Int32:
    case ezVariantType::UInt32:
    case ezVariantType::Int64:
    case ezVariantType::UInt64:
    case ezVariantType::Float:
    {
      const double value = var.ConvertTo<double>();
      var = value;
      return;
    }

    default:
      return;
  }
}

static ezUInt32 s_StorageSizes[] = {
  ezInvalidIndex,             // None
  sizeof(double),             // Number
  sizeof(bool),               // Boolean
  sizeof(ezVec3),             // Vec3
  sizeof(ezString),           // String
  sizeof(ezGameObjectHandle), // GameObjectHandle
  sizeof(ezComponentHandle),  // ComponentHandle
  sizeof(ezVariant),          // Variant
};

// static
ezUInt32 ezVisualScriptDataPinType::GetStorageByteSize(Enum dataPinType)
{
  if (dataPinType >= Number && dataPinType <= Variant)
  {
    EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(s_StorageSizes) == Variant + 1);
    return s_StorageSizes[dataPinType];
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezInvalidIndex;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinOutAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinInAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinInAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptDataPinType, m_DataType)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinOutAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptDataPinType, m_DataType)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode::ezVisualScriptNode() {}
ezVisualScriptNode::~ezVisualScriptNode() {}


ezInt32 ezVisualScriptNode::HandlesMessagesWithID() const
{
  return -1;
}

void ezVisualScriptNode::HandleMessage(ezMessage* pMsg) {}

bool ezVisualScriptNode::IsManuallyStepped() const
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);

  for (auto prop : properties)
  {
    if (prop->GetAttributeByType<ezVisScriptExecPinOutAttribute>() != nullptr)
      return true;

    if (prop->GetAttributeByType<ezVisScriptExecPinInAttribute>() != nullptr)
      return true;
  }

  return false;
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
    ezHybridArray<ezAbstractProperty*, 32> properties;
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

      ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);
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
            ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);

            const ezRTTI* pType = pAbsMember->GetSpecificType();
            if (ezVisualScriptDataPinType::IsTypeSupported(pType))
            {
              ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow());
              ezVisualScriptDataPinType::EnforceSupportedType(var);
              pInstance->SetOutputPinValue(this, uiProp, var.GetData());
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

  if (m_pMessageToSend != nullptr)
  {
    const ezUInt32 uiProp = uiPin - 2;

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

  ezHybridArray<ezAbstractProperty*, 32> properties;
  m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

  const ezUInt8 uiPropCount = static_cast<ezUInt8>(properties.GetCount());
  m_PropertyIndexToMemoryOffset.SetCount(uiPropCount, 0xFFFF);
  m_PropertyIndexToDataPinType.SetCount(uiPropCount);

  for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    if (properties[uiProp]->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);

      const ezRTTI* pType = pAbsMember->GetSpecificType();
      auto dataPinType = ezVisualScriptDataPinType::GetDataPinTypeForType(pType);
      if (dataPinType == ezVisualScriptDataPinType::None)
        continue;

      m_PropertyIndexToMemoryOffset[uiProp] = uiScratchMemorySize;
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

    ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);
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

ezVisualScriptNode_MessageHandler::ezVisualScriptNode_MessageHandler() {}
ezVisualScriptNode_MessageHandler::~ezVisualScriptNode_MessageHandler() {}

void ezVisualScriptNode_MessageHandler::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_pMsgCopy == nullptr)
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  m_pMsgCopy->GetDynamicRTTI()->GetAllProperties(properties);

  const ezUInt8 uiPropCount = static_cast<ezUInt8>(properties.GetCount());
  for (ezUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    auto prop = properties[uiProp];

    if (prop->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(prop);

      const ezRTTI* pType = pAbsMember->GetSpecificType();
      if (ezVisualScriptDataPinType::IsTypeSupported(pType))
      {
        ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMsgCopy.Borrow());
        ezVisualScriptDataPinType::EnforceSupportedType(var);
        pInstance->SetOutputPinValue(this, uiProp, var.GetData());
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

ezVisualScriptNode_FunctionCall::ezVisualScriptNode_FunctionCall() {}

ezVisualScriptNode_FunctionCall::~ezVisualScriptNode_FunctionCall() {}

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
    const ezVariant& var = m_Arguments[arg];
    const ezVariantType::Enum targetType = m_pFunctionToCall->GetArgumentType(arg)->GetVariantType();

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
    pInstance->SetOutputPinValue(this, uiOutputPinIndex, m_ReturnValue.GetData());
    ++uiOutputPinIndex;
  }

  for (ezUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    // also do this for non-out parameters, as an 'in' parameter may still be a non-const reference (bad but valid)
    ezVisualScriptDataPinType::EnforceSupportedType(m_Arguments[arg]);

    if ((m_ArgumentIsOutParamMask & EZ_BIT(arg)) != 0) // if this argument represents an out or inout parameter, pull the data
    {
      pInstance->SetOutputPinValue(this, uiOutputPinIndex, m_Arguments[arg].GetData());
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

ezResult ezVisualScriptNode_FunctionCall::ConvertArgumentToRequiredType(ezVariant& var, ezVariantType::Enum type)
{
  if (var.GetType() == type)
    return EZ_SUCCESS;

  ezResult couldConvert = EZ_FAILURE;
  var = var.ConvertTo(type, &couldConvert);

  return couldConvert;
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
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new ezVisScriptDataPinInAttribute(1, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value3", m_Value3)->AddAttributes(new ezVisScriptDataPinInAttribute(2, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    // Properties
    EZ_MEMBER_PROPERTY("Text", m_sLog)->AddAttributes(new ezDefaultValueAttribute(ezStringView("Value1: {0}, Value2: {1}, Value3: {2}"))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Log::ezVisualScriptNode_Log() {}
ezVisualScriptNode_Log::~ezVisualScriptNode_Log() {}

void ezVisualScriptNode_Log::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezLog::Info(m_sLog, m_Value1, m_Value2, m_Value3);

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_Log::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value1;
    case 1:
      return &m_Value2;
    case 2:
      return &m_Value3;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptNode);
