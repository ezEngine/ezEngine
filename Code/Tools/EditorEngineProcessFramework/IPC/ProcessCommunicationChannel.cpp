#include <PCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Logging/Log.h>

ezProcessCommunicationChannel::ezProcessCommunicationChannel() {}

ezProcessCommunicationChannel::~ezProcessCommunicationChannel()
{
  if (m_pChannel)
  {
    EZ_DEFAULT_DELETE(m_pChannel);
  }
}

void ezProcessCommunicationChannel::SendMessage(ezProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
      return;

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pChannel == nullptr)
      return;

    m_pChannel->Send(pMessage);
  }
}

bool ezProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pChannel)
    return false;

  return m_pChannel->ProcessMessages();
}


void ezProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pChannel)
    return;

  m_pChannel->WaitForMessages();
}

void ezProcessCommunicationChannel::MessageFunc(const ezProcessMessage* msg)
{
  const ezRTTI* pRtti = msg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && msg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<ezProcessMessage*>(msg)))
      {
        m_WaitForMessageCallback = WaitForMessageCallback();
        m_pWaitForMessageType = nullptr;
      }
    }
    else
    {
      m_pWaitForMessageType = nullptr;
    }
  }

  EZ_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
  EZ_ASSERT_DEV(msg != nullptr, "Object could not be allocated");
  EZ_ASSERT_DEV(pRtti->IsDerivedFrom<ezProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = msg;
  m_Events.Broadcast(e);
}

ezResult ezProcessCommunicationChannel::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout, WaitForMessageCallback* pMessageCallack)
{
  EZ_ASSERT_DEV(m_pChannel != nullptr, "Need to connect first before waiting for a message.");
  // EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function is not thread safe");
  EZ_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;
  if (pMessageCallack)
  {
    m_WaitForMessageCallback = *pMessageCallack;
  }
  else
  {
    m_WaitForMessageCallback = WaitForMessageCallback();
  }

  const ezTime tStart = ezTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    m_pChannel->WaitForMessages();

    if (tTimeout != ezTime())
    {
      if (ezTime::Now() - tStart > tTimeout)
      {
        m_pWaitForMessageType = nullptr;
        ezLog::Error("Reached time-out of {0} seconds while waiting for {1}", ezArgF(tTimeout.GetSeconds(), 1),
                     pMessageType->GetTypeName());
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}
