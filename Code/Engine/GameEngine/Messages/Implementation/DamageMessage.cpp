#include <PCH.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezDamageMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDamageMessage, 1, ezRTTIDefaultAllocator<ezDamageMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_DamageEvent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_DamageEvent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Events/Gameplay")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_OUTPUT_EXECUTION_PIN("OnEvent", 0),
    // Data Pins
    EZ_OUTPUT_DATA_PIN("Damage", 0, ezVisualScriptDataPinType::Number)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_DamageEvent::ezVisualScriptNode_DamageEvent() { }
ezVisualScriptNode_DamageEvent::~ezVisualScriptNode_DamageEvent() { }

void ezVisualScriptNode_DamageEvent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(m_Msg.m_fDamage) == 8, "The damage value is directly used by a visual script node, so it must be a double.");

  pInstance->SetOutputPinValue(this, 0, &m_Msg.m_fDamage);
  pInstance->ExecuteConnectedNodes(this, 0);
}

ezInt32 ezVisualScriptNode_DamageEvent::HandlesMessagesWithID() const
{
  return ezDamageMessage::GetTypeMsgId();
}

void ezVisualScriptNode_DamageEvent::HandleMessage(ezMessage* pMsg)
{
  ezDamageMessage& msg = *static_cast<ezDamageMessage*>(pMsg);

  m_Msg = msg;
  m_bStepNode = true;
}
