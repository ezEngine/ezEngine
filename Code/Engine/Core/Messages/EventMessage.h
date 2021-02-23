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
    static void SendEventMessage(ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg);
    static void SendEventMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg);
    static void PostEventMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezTime delay,
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
  EZ_ALWAYS_INLINE ezEventMessageSender() { *reinterpret_cast<ezUInt64*>(&m_hCachedReceiver) = 0xFFFFFFFFFFFFFFFF; }

  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, ezComponent* pSenderComponent, const ezGameObject* pSearchObject)
  {
    UpdateMessageAndCachedReceiver(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_hCachedReceiver, msg);
  }

  EZ_ALWAYS_INLINE void SendEventMessage(EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    UpdateMessageAndCachedReceiver(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendEventMessage(pSenderComponent, m_hCachedReceiver, msg);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(const EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType) const
  {
    UpdateMessageAndCachedReceiver(const_cast<EventMessageType&>(msg), pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::PostEventMessage(pSenderComponent, m_hCachedReceiver, msg, delay, queueType);
  }

private:
  void UpdateMessageAndCachedReceiver(ezEventMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    msg.m_hSenderObject = pSenderComponent->GetOwner() != nullptr ? pSenderComponent->GetOwner()->GetHandle() : ezGameObjectHandle();
    msg.m_hSenderComponent = pSenderComponent->GetHandle();

    if (*reinterpret_cast<ezUInt64*>(&m_hCachedReceiver) == 0xFFFFFFFFFFFFFFFF)
    {
      ezHybridArray<const ezComponent*, 4> eventMsgHandlers;
      pSenderComponent->GetWorld()->FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      if (eventMsgHandlers.IsEmpty() == false)
      {
        m_hCachedReceiver = eventMsgHandlers[0]->GetHandle();
      }
      else
      {
        m_hCachedReceiver.Invalidate();
      }
    }
  }

  mutable ezComponentHandle m_hCachedReceiver;
};
