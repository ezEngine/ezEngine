#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/GameObject.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all messages that are sent as 'events'
struct EZ_CORE_DLL ezEventMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezEventMessage, ezMessage);

  ezGameObjectHandle m_hSenderObject;
  ezComponentHandle m_hSenderComponent;
};

namespace ezInternal
{
  struct EZ_CORE_DLL EventMessageSenderHelper
  {
    static void SendEventMessage(ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, ezEventMessage& msg);
    static void SendEventMessage(const ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, ezEventMessage& msg);
    static void PostEventMessage(const ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, const ezEventMessage& msg, ezTime delay,
      ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame);
  };
} // namespace ezInternal

/// \brief A message sender that sends all messages to the next component derived from ezEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class ezEventMessageSender : public ezMessageSenderBase<EventMessageType>
{
public:
  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, ezComponent* pSenderComponent, const ezGameObject* pSearchObject)
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_CachedReceivers, msg);
  }

  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    UpdateMessageAndCachedReceivers(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_CachedReceivers, msg);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(const EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType) const
  {
    UpdateMessageAndCachedReceivers(const_cast<EventMessageType&>(msg), pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::PostEventMessage(pSenderComponent, m_CachedReceivers, msg, delay, queueType);
  }

  EZ_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<ezUInt32>() = 0;
  }

private:
  void UpdateMessageAndCachedReceivers(ezEventMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    msg.m_hSenderObject = pSenderComponent->GetOwner() != nullptr ? pSenderComponent->GetOwner()->GetHandle() : ezGameObjectHandle();
    msg.m_hSenderComponent = pSenderComponent->GetHandle();

    if (m_CachedReceivers.GetUserData<ezUInt32>() == 0)
    {
      ezHybridArray<const ezComponent*, 4> eventMsgHandlers;
      pSenderComponent->GetWorld()->FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        m_CachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      m_CachedReceivers.GetUserData<ezUInt32>() = 1;
    }
  }

  mutable ezSmallArray<ezComponentHandle, 1> m_CachedReceivers;
};
