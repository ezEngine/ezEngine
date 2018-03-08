#include <PCH.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDamage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDamage, 1, ezRTTIDefaultAllocator<ezMsgDamage>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Damage", m_fDamage)
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
    new ezAutoGenVisScriptMsgHandler
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_OnDamage, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_OnDamage>)
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

ezVisualScriptNode_OnDamage::ezVisualScriptNode_OnDamage() { }
ezVisualScriptNode_OnDamage::~ezVisualScriptNode_OnDamage() { }

void ezVisualScriptNode_OnDamage::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(m_Msg.m_fDamage) == 8, "The damage value is directly used by a visual script node, so it must be a double.");

  pInstance->SetOutputPinValue(this, 0, &m_Msg.m_fDamage);
  pInstance->ExecuteConnectedNodes(this, 0);
}

ezInt32 ezVisualScriptNode_OnDamage::HandlesMessagesWithID() const
{
  return ezMsgDamage::GetTypeMsgId();
}

void ezVisualScriptNode_OnDamage::HandleMessage(ezMessage* pMsg)
{
  ezMsgDamage& msg = *static_cast<ezMsgDamage*>(pMsg);

  m_Msg = msg;
  m_bStepNode = true;
}
