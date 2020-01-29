#include <CorePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgCollision);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgCollision, 1, ezRTTIDefaultAllocator<ezMsgCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTriggerState, 1)
  EZ_ENUM_CONSTANTS(ezTriggerState::Activated, ezTriggerState::Continuing, ezTriggerState::Deactivated)
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDeleteGameObject);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDeleteGameObject, 1, ezRTTIDefaultAllocator<ezMsgDeleteGameObject>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgComponentInternalTrigger);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgComponentInternalTrigger, 1, ezRTTIDefaultAllocator<ezMsgComponentInternalTrigger>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("UsageStringHash", m_uiUsageStringHash)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender(),
    new ezAutoGenVisScriptMsgHandler()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgUpdateLocalBounds);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgUpdateLocalBounds, 1, ezRTTIDefaultAllocator<ezMsgUpdateLocalBounds>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgChildrenChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgChildrenChanged, 1, ezRTTIDefaultAllocator<ezMsgChildrenChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgComponentsChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgComponentsChanged, 1, ezRTTIDefaultAllocator<ezMsgComponentsChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgTransformChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgTransformChanged, 1, ezRTTIDefaultAllocator<ezMsgTransformChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetFloatParameter);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetFloatParameter, 1, ezRTTIDefaultAllocator<ezMsgSetFloatParameter>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sParameterName),
    EZ_MEMBER_PROPERTY("Value", m_fValue),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgGenericEvent);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgGenericEvent, 1, ezRTTIDefaultAllocator<ezMsgGenericEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationReachedEnd);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationReachedEnd, 1, ezRTTIDefaultAllocator<ezMsgAnimationReachedEnd>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgHandler(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

// clang-format on

EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
