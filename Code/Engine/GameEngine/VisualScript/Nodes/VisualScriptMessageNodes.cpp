#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMessageNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

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

