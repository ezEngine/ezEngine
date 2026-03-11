#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

namespace ezInternal
{
  struct EZ_CORE_DLL EventMessageSenderHelper
  {
    static bool SendEventMessage(ezMessage& ref_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers);
    static bool SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers);
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
  EZ_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject)
  {
    return ezInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  EZ_ALWAYS_INLINE bool SendEventMessage(EventMessageType& inout_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject) const
  {
    return ezInternal::EventMessageSenderHelper::SendEventMessage(inout_msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame)
  {
    ezInternal::EventMessageSenderHelper::PostEventMessage(ref_msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  EZ_ALWAYS_INLINE void PostEventMessage(EventMessageType& ref_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject,
    ezTime delay, ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const
  {
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
