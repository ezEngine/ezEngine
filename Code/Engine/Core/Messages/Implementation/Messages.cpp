#include <PCH.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/ScriptFunctionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezCollisionMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMessage, 1, ezRTTIDefaultAllocator<ezCollisionMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezDeleteObjectMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDeleteObjectMessage, 1, ezRTTIDefaultAllocator<ezDeleteObjectMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezInternalComponentMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInternalComponentMessage, 1, ezRTTIDefaultAllocator<ezInternalComponentMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezScriptFunctionMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptFunctionMessage, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezUpdateLocalBoundsMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpdateLocalBoundsMessage, 1, ezRTTIDefaultAllocator<ezUpdateLocalBoundsMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE




EZ_STATICLINK_FILE(Core, Core_Messages_Messages);

