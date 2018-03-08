#pragma once

#include <Foundation/Communication/Message.h>
#include <Core/World/GameObject.h>

/// \brief Base class for all messages that are sent as 'events'
struct EZ_CORE_DLL ezEventMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezEventMessage, ezMessage);

  ezGameObjectHandle m_hSenderObject;
  ezComponentHandle m_hSenderComponent;
};

/// \brief For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom messages for more elaborate cases where a string is not sufficient information.
/// Also be aware that passing this message is not the most efficient due to the string copy overhead.
struct EZ_CORE_DLL ezMsgGenericUserEvent : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgGenericUserEvent, ezEventMessage);

  /// A custom string to identify the intent.
  ezString m_sMessage;
};

namespace ezInternal
{
  struct EZ_CORE_DLL EventMessageSenderHelper
  {
    static ezComponentHandle FindReceiver(ezEventMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject);

    static void SendMessage(ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg);
    static void SendMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg);
    static void PostMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezObjectMsgQueueType::Enum queueType);
    static void PostMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay);
  };
}

/// \brief A message sender that sends all messages to the next component derived from ezEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class ezEventMessageSender : public ezMessageSenderBase<EventMessageType>
{
public:
  EZ_ALWAYS_INLINE ezEventMessageSender()
  {
    *reinterpret_cast<ezUInt64*>(&m_hCachedReceiver) = 0xFFFFFFFFFFFFFFFF;
  }

  EZ_ALWAYS_INLINE void SendMessage(EventMessageType& msg, ezComponent* pSenderComponent, const ezGameObject* pSearchObject)
  {
    UpdateMessageAndCachedReceiver(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendMessage(pSenderComponent, m_hCachedReceiver, msg);
  }

  EZ_ALWAYS_INLINE void SendMessage(EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    UpdateMessageAndCachedReceiver(msg, pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::SendMessage(pSenderComponent, m_hCachedReceiver, msg);
  }

  EZ_ALWAYS_INLINE void PostMessage(const EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezObjectMsgQueueType::Enum queueType) const
  {
    UpdateMessageAndCachedReceiver(const_cast<EventMessageType&>(msg), pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::PostMessage(pSenderComponent, m_hCachedReceiver, msg, queueType);
  }

  EZ_ALWAYS_INLINE void PostMessage(const EventMessageType& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezObjectMsgQueueType::Enum queueType, ezTime delay) const
  {
    UpdateMessageAndCachedReceiver(const_cast<EventMessageType&>(msg), pSenderComponent, pSearchObject);

    ezInternal::EventMessageSenderHelper::PostMessage(pSenderComponent, m_hCachedReceiver, msg, queueType, delay);
  }

private:
  EZ_ALWAYS_INLINE void UpdateMessageAndCachedReceiver(ezEventMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    msg.m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    msg.m_hSenderComponent = pSenderComponent->GetHandle();

    if (*reinterpret_cast<ezUInt64*>(&m_hCachedReceiver) == 0xFFFFFFFFFFFFFFFF)
    {
      m_hCachedReceiver = ezInternal::EventMessageSenderHelper::FindReceiver(msg, pSenderComponent, pSearchObject);
    }
  }

  mutable ezComponentHandle m_hCachedReceiver;
};

