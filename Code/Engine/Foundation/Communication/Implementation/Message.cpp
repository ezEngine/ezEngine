#include <PCH.h>
#include <Foundation/Communication/Message.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMessage, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezScriptMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptMessage, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMessageId ezMessage::s_uiNextMsgId = 0;


EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);

