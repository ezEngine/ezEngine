#include <PhysXPluginPCH.h>

#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <PhysXPlugin/Components/PxTriggerComponent.h>
#include <PhysXPlugin/VisualScriptNodes/PxVisualScriptNodes.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_PxTriggerEvent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_PxTriggerEvent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Events"),
    new ezTitleAttribute("Trigger Event '{TriggerMessage}'"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
    EZ_OUTPUT_EXECUTION_PIN("OnActivated", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnDeactivated", 2),
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_PxTriggerEvent::ezVisualScriptNode_PxTriggerEvent() {}
ezVisualScriptNode_PxTriggerEvent::~ezVisualScriptNode_PxTriggerEvent() {}

void ezVisualScriptNode_PxTriggerEvent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_State == ezTriggerState::Activated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hObject);
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else if (m_State == ezTriggerState::Deactivated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hObject);
    pInstance->ExecuteConnectedNodes(this, 2);
  }
}


ezInt32 ezVisualScriptNode_PxTriggerEvent::HandlesMessagesWithID() const
{
  return ezMsgTriggerTriggered::GetTypeMsgId();
}

void ezVisualScriptNode_PxTriggerEvent::HandleMessage(ezMessage* pMsg)
{
  ezMsgTriggerTriggered& msg = *static_cast<ezMsgTriggerTriggered*>(pMsg);

  if (msg.m_uiMessageStringHash == m_sTriggerMessage.GetHash())
  {
    m_bStepNode = true;

    m_State = msg.m_TriggerState;
    m_hObject = msg.m_hTriggeringObject;
  }
}

//////////////////////////////////////////////////////////////////////////
