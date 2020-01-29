
#include <GameEnginePCH.h>

#include <GameEngine/Messages/TriggerTriggeredMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgTriggerTriggered)
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgTriggerTriggered, 1, ezRTTIDefaultAllocator<ezMsgTriggerTriggered>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MsgStringHash", m_uiMessageStringHash),
    EZ_ENUM_MEMBER_PROPERTY("TriggerState", ezTriggerState, m_TriggerState),
    //EZ_MEMBER_PROPERTY("GameObject", m_hTriggeringObject),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
