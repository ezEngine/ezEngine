#include <PCH.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgCollision);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgCollision, 1, ezRTTIDefaultAllocator<ezMsgCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDeleteGameObject);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDeleteGameObject, 1, ezRTTIDefaultAllocator<ezMsgDeleteGameObject>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgComponentInternalTrigger);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgComponentInternalTrigger, 1, ezRTTIDefaultAllocator<ezMsgComponentInternalTrigger>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgUpdateLocalBounds);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgUpdateLocalBounds, 1, ezRTTIDefaultAllocator<ezMsgUpdateLocalBounds>)
EZ_END_DYNAMIC_REFLECTED_TYPE




EZ_STATICLINK_FILE(Core, Core_Messages_Messages);

