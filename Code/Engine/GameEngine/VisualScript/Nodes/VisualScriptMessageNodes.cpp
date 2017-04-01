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


