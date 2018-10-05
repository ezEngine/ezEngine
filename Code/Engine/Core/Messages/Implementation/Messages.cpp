#include <PCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgCollision);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgCollision, 1, ezRTTIDefaultAllocator<ezMsgCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDeleteGameObject);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDeleteGameObject, 1, ezRTTIDefaultAllocator<ezMsgDeleteGameObject>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgComponentInternalTrigger);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgComponentInternalTrigger, 1, ezRTTIDefaultAllocator<ezMsgComponentInternalTrigger>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender(),
    new ezAutoGenVisScriptMsgHandler()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgUpdateLocalBounds);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgUpdateLocalBounds, 1, ezRTTIDefaultAllocator<ezMsgUpdateLocalBounds>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetPlaying);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetPlaying, 1, ezRTTIDefaultAllocator<ezMsgSetPlaying>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
