#include <GameEnginePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptDataPinType, 1)
  EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::None, ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Vec3)
  EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptDataPinType::ComponentHandle)
EZ_END_STATIC_REFLECTED_ENUM;

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

ezVisualScriptNode_MessageSender::ezVisualScriptNode_MessageSender()
{
  m_bRecursive = false;
}

ezVisualScriptNode_MessageSender::~ezVisualScriptNode_MessageSender()
{
  if (m_pMessageToSend != nullptr)
  {
    m_pMessageToSend->GetDynamicRTTI()->GetAllocator()->Deallocate(m_pMessageToSend);
    m_pMessageToSend = nullptr;
  }
}

void ezVisualScriptNode_MessageSender::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_pMessageToSend != nullptr)
  {
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

        ezHybridArray<ezAbstractProperty*, 32> properties;
        m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

        for (ezUInt32 uiProp = 0; uiProp < properties.GetCount(); ++uiProp)
        {
          if (properties[uiProp]->GetCategory() == ezPropertyCategory::Member &&
              properties[uiProp]->GetFlags().IsAnySet(ezPropertyFlags::VarInOut | ezPropertyFlags::VarOut))
          {
            ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);

            const ezRTTI* pType = pAbsMember->GetSpecificType();

            if (pType == ezGetStaticRTTI<bool>() || pType == ezGetStaticRTTI<double>() || pType == ezGetStaticRTTI<ezVec3>())
            {
              const void* pPropPtr = pAbsMember->GetPropertyPointer(m_pMessageToSend);
              pInstance->SetOutputPinValue(this, uiProp, pPropPtr);
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
        pWorld->PostMessage(m_hComponent, *m_pMessageToSend, ezObjectMsgQueueType::NextFrame, m_Delay);
      }
      else
      {
        ezGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;
        if (m_bRecursive)
        {
          pWorld->PostMessageRecursive(hObject, *m_pMessageToSend, ezObjectMsgQueueType::NextFrame, m_Delay);
        }
        else
        {
          pWorld->PostMessage(hObject, *m_pMessageToSend, ezObjectMsgQueueType::NextFrame, m_Delay);
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

    ezHybridArray<ezAbstractProperty*, 32> properties;
    m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

    if (properties[uiProp]->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pAbsMember = static_cast<ezAbstractMemberProperty*>(properties[uiProp]);

      const ezRTTI* pType = pAbsMember->GetSpecificType();

      if (pType == ezGetStaticRTTI<bool>() || pType == ezGetStaticRTTI<double>() || pType == ezGetStaticRTTI<ezVec3>())
      {
        return pAbsMember->GetPropertyPointer(m_pMessageToSend);
      }
    }
  }

  return nullptr;
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

ezVisualScriptNode_FunctionCall::ezVisualScriptNode_FunctionCall()
{
}

ezVisualScriptNode_FunctionCall::~ezVisualScriptNode_FunctionCall()
{
}

void ezVisualScriptNode_FunctionCall::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated() || m_pFunctionToCall == nullptr)
    return;

  ezWorld* pWorld = pInstance->GetWorld();

  ezComponent* pComponent = nullptr;
  if (!pWorld->TryGetComponent(m_hComponent, pComponent))
    return;

  if (!pComponent->GetDynamicRTTI()->IsDerivedFrom(m_pExpectedType))
  {
    m_pFunctionToCall = nullptr;
    ezLog::Error("Component of type '{}' is not of the expected type '{}'", pComponent->GetDynamicRTTI()->GetTypeName(), m_pExpectedType->GetTypeName());
    return;
  }

  m_pFunctionToCall->Execute(pComponent, m_Arguments, m_ReturnValue);



  for (ezUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    if (m_pFunctionToCall->GetArgumentFlags(arg).IsSet(ezPropertyFlags::Reference)) // TODO: does reference mean non-const ?
    {
      // TODO: reset out parameters to the proper type and pass them on

      //switch (m_pFunctionToCall->GetArgumentType(arg)->GetVariantType())
      //{
      //  case ezVariantType::Bool:
      //    m_Arguments[arg] = false;
      //    break;
      //  case ezVariantType::Double:
      //  case ezVariantType::Float:
      //  case ezVariantType::Int8:
      //  case ezVariantType::Int16:
      //  case ezVariantType::Int32:
      //  case ezVariantType::UInt8:
      //  case ezVariantType::UInt16:
      //  case ezVariantType::UInt32:
      //    m_Arguments[arg] = static_cast<double>(0);
      //    break;
      //  case ezVariantType::Vector3:
      //    m_Arguments[arg] = ezVec3::ZeroVector();
      //    break;

      //  default:
      //    ezLog::Error("Script function '{}' has unsupported argument type '{}' for parameter {}", node.m_sTypeName, pFunc->GetArgumentType(arg)->GetVariantType(), arg);
      //    break;
      //}

      // TODO: pass out parameters to output pins
      // TODO: distinguish between in / out /inout parameters
      // TODO: pass return value to output pin
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_FunctionCall::GetInputPinDataPointer(ezUInt8 uiPin)
{
  if (uiPin == 0)
    return &m_hComponent;

  if (uiPin >= m_Arguments.GetCount() + 1)
    return &m_ReturnValue; // unused dummy

  return m_Arguments[uiPin - 1].GetData();
}


void ezVisualScriptNode_FunctionCall::EnforceVariantTypes()
{
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
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Properties
    EZ_MEMBER_PROPERTY("Text", m_sLog)->AddAttributes(new ezDefaultValueAttribute("Value1: {0}, Value2: {1}")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Log::ezVisualScriptNode_Log() {}
ezVisualScriptNode_Log::~ezVisualScriptNode_Log() {}

void ezVisualScriptNode_Log::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezLog::Dev(m_sLog, m_Value1, m_Value2);

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
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptNode);
