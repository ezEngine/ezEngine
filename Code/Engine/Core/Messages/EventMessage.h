#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all messages that are sent as 'events'
struct EZ_CORE_DLL ezEventMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezEventMessage, ezMessage);

  ezGameObjectHandle m_hSenderObject;
  ezComponentHandle m_hSenderComponent;

  EZ_ALWAYS_INLINE void FillFromSenderComponent(const ezComponent* pSenderComponent)
  {
    if (pSenderComponent != nullptr)
    {
      m_hSenderComponent = pSenderComponent->GetHandle();
      m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    }
  }
};

namespace ezInternal
{
  struct EZ_CORE_DLL EventMessageSenderHelper
  {
    static void SendEventMessage(ezMessage& ref_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers);
    static void SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers);
    static void PostEventMessage(const ezMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers, ezTime delay, ezObjectMsgQueueType::Enum queueType);
  };
} // namespace ezInternal

/// \brief A message sender that sends all messages to the next component derived from ezEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class ezEventMessageSender : public ezMessageSenderBase<EventMessageType>
{
public:
  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& inout_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject)
  {
    if constexpr (EZ_IS_DERIVED_FROM_STATIC(ezEventMessage, EventMessageType))
    {
      inout_msg.FillFromSenderComponent(pSenderComponent);
    }
    ezInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& inout_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    if constexpr (EZ_IS_DERIVED_FROM_STATIC(ezEventMessage, EventMessageType))
    {
      inout_msg.FillFromSenderComponent(pSenderComponent);
    }
    ezInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame)
  {
    if constexpr (EZ_IS_DERIVED_FROM_STATIC(ezEventMessage, EventMessageType))
    {
      ref_msg.FillFromSenderComponent(pSenderComponent);
    }
    ezInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const
  {
    if constexpr (EZ_IS_DERIVED_FROM_STATIC(ezEventMessage, EventMessageType))
    {
      ref_msg.FillFromSenderComponent(pSenderComponent);
    }
    ezInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  EZ_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<ezUInt32>() = 0;
  }

private:
  mutable ezSmallArray<ezComponentHandle, 1> m_CachedReceivers;
};
