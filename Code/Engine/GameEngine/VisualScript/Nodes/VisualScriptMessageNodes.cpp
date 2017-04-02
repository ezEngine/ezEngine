#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMessageNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/World/World.h>
#include <Components/InputComponent.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_OnUserTriggerMsg, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_OnUserTriggerMsg>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Message Handler")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    //Properties
    EZ_ACCESSOR_PROPERTY("Message", GetMessage, SetMessage),
    // Execution Pins
    EZ_OUTPUT_EXECUTION_PIN("OnMsg", 0),
    // Data Pins
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUserTriggerMessage, OnUserTriggerMsg),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_OnUserTriggerMsg::ezVisualScriptNode_OnUserTriggerMsg() { }
ezVisualScriptNode_OnUserTriggerMsg::~ezVisualScriptNode_OnUserTriggerMsg() { }

void ezVisualScriptNode_OnUserTriggerMsg::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
}

void ezVisualScriptNode_OnUserTriggerMsg::OnUserTriggerMsg(ezUserTriggerMessage& msg)
{
  if (msg.m_sMessage == m_sMessage)
  {
    m_bStepNode = true;
    ezLog::Debug("User Trigger Msg '{0}' arrived", m_sMessage);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_OnTriggerMsg, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_OnTriggerMsg>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Messages/Trigger")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
    EZ_OUTPUT_EXECUTION_PIN("OnMsg", 0),
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES

    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezTriggerMessage, TriggerMessageHandler),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_OnTriggerMsg::ezVisualScriptNode_OnTriggerMsg() { }
ezVisualScriptNode_OnTriggerMsg::~ezVisualScriptNode_OnTriggerMsg() { }

void ezVisualScriptNode_OnTriggerMsg::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->SetOutputPinValue(this, 0, &m_hObject);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void ezVisualScriptNode_OnTriggerMsg::TriggerMessageHandler(ezTriggerMessage& msg)
{
  if (msg.m_UsageStringHash == m_sTriggerMessage.GetHash())
  {
    m_bStepNode = true;
    m_hObject = msg.m_hTriggeringObject;
    ezLog::Debug("Trigger Msg '{0}' arrived", m_sTriggerMessage.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_OnScriptUpdate, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_OnScriptUpdate>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    EZ_OUTPUT_EXECUTION_PIN("OnUpdate", 0),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_OnScriptUpdate::ezVisualScriptNode_OnScriptUpdate()
{
  m_bStepNode = true;
}

ezVisualScriptNode_OnScriptUpdate::~ezVisualScriptNode_OnScriptUpdate() { }

void ezVisualScriptNode_OnScriptUpdate::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);

  // Make sure to be updated again next frame
  m_bStepNode = true;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_InputState, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_InputState>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputAction", m_sInputAction),
    EZ_MEMBER_PROPERTY("OnlyKeyPressed", m_bOnlyKeyPressed),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_InputState::ezVisualScriptNode_InputState() { }
ezVisualScriptNode_InputState::~ezVisualScriptNode_InputState() { }

void ezVisualScriptNode_InputState::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated())
    return;

  ezComponent* pComponent;
  if (!pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    m_hComponent.Invalidate();
    return;
  }

  ezInputComponent* pInput = ezDynamicCast<ezInputComponent*>(pComponent);
  if (pInput == nullptr)
  {
    m_hComponent.Invalidate();
    return;
  }

  const double fValue = pInput->GetCurrentInputState(m_sInputAction, m_bOnlyKeyPressed);
  pInstance->SetOutputPinValue(this, 0, &fValue);
}

void* ezVisualScriptNode_InputState::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_hComponent;
}

