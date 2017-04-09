#include <PCH.h>
#include <Core/Messages/CallDelayedStartMessage.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezCallDelayedStartMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCallDelayedStartMessage, 1, ezRTTIDefaultAllocator<ezCallDelayedStartMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezCollisionMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMessage, 1, ezRTTIDefaultAllocator<ezCollisionMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezDeleteObjectMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDeleteObjectMessage, 1, ezRTTIDefaultAllocator<ezDeleteObjectMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezInternalComponentMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInternalComponentMessage, 1, ezRTTIDefaultAllocator<ezInternalComponentMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezSimpleUserEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleUserEventMessage, 1, ezRTTIDefaultAllocator<ezSimpleUserEventMessage>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezUpdateLocalBoundsMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateLocalBoundsMessage, 1, ezRTTIDefaultAllocator<ezUpdateLocalBoundsMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE




EZ_STATICLINK_FILE(Core, Core_Messages_Messages);

