#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

ezProcessCommunicationChannel::ezProcessCommunicationChannel() = default;

ezProcessCommunicationChannel::~ezProcessCommunicationChannel()
{
  m_pProtocol.Clear();
  m_pChannel.Clear();
}

bool ezProcessCommunicationChannel::SendMessage(ezProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
    {
      ezLog::Warning("[IPC]Ignored send message of type {} because it is not a {}", pMessage->GetDynamicRTTI()->GetTypeName(), m_pFirstAllowedMessageType->GetTypeName());
      return false;
    }

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pProtocol == nullptr)
      return false;

    return m_pProtocol->Send(pMessage);
  }
}

bool ezProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pProtocol)
    return false;

  return m_pProtocol->ProcessMessages();
}


void ezProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pProtocol)
    return;

  m_pProtocol->WaitForMessages().IgnoreResult();
}

void ezProcessCommunicationChannel::MessageFunc(const ezIpcProcessMessageProtocol::Event& msg)
{
  const ezProcessMessage* pMsg = msg.m_pMessage;
  const ezRTTI* pRtti = pMsg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && pMsg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<ezProcessMessage*>(pMsg)))
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
  EZ_ASSERT_DEV(pMsg != nullptr, "Object could not be allocated");
  EZ_ASSERT_DEV(pRtti->IsDerivedFrom<ezProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = pMsg;
  e.m_bInterruptMessageProcessing = msg.m_bInterruptMessageProcessing;
  m_Events.Broadcast(e);
  msg.m_bInterruptMessageProcessing = e.m_bInterruptMessageProcessing;
}

ezResult ezProcessCommunicationChannel::WaitForMessage(const ezRTTI* pMessageType, ezTime timeout, WaitForMessageCallback* pMessageCallack)
{
  EZ_ASSERT_DEV(m_pProtocol != nullptr && m_pChannel != nullptr, "Need to connect first before waiting for a message.");
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

  EZ_SCOPE_EXIT(m_WaitForMessageCallback = WaitForMessageCallback(););

  const ezTime tStart = ezTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    if (timeout == ezTime())
    {
      m_pProtocol->WaitForMessages().IgnoreResult();
    }
    else
    {
      ezTime tTimeLeft = timeout - (ezTime::Now() - tStart);

      if (tTimeLeft < ezTime::MakeZero())
      {
        // Don't time out if a debugger is attached to make stepping easier.
        if (ezSystemInformation::IsDebuggerAttached())
        {
          tTimeLeft = ezTime::MakeFromSeconds(1);
        }
        else
        {
          m_pWaitForMessageType = nullptr;
          ezLog::Dev("Reached time-out of {0} seconds while waiting for {1}", ezArgF(timeout.GetSeconds(), 1), pMessageType->GetTypeName());
          return EZ_FAILURE;
        }
      }

      m_pProtocol->WaitForMessages(tTimeLeft).IgnoreResult();
    }

    if (!m_pChannel->IsConnected())
    {
      m_pWaitForMessageType = nullptr;
      ezLog::Dev("Lost connection while waiting for {}", pMessageType->GetTypeName());
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezProcessCommunicationChannel::WaitForConnection(ezTime timeout)
{
  if (m_pChannel->IsConnected())
  {
    return EZ_SUCCESS;
  }

  ezThreadSignal waitForConnectionSignal;

  ezEventSubscriptionID eventSubscriptionId = m_pChannel->m_Events.AddEventHandler([&](const ezIpcChannelEvent& event)
    {
    switch (event.m_Type)
    {
      case ezIpcChannelEvent::Connected:
      case ezIpcChannelEvent::Disconnected:
        waitForConnectionSignal.RaiseSignal();
        break;
      default:
        break;
    } });

  EZ_SCOPE_EXIT(m_pChannel->m_Events.RemoveEventHandler(eventSubscriptionId));

  if (m_pChannel->IsConnected())
  {
    return EZ_SUCCESS;
  }

  if (timeout == ezTime())
  {
    waitForConnectionSignal.WaitForSignal();
  }
  else
  {
    if (waitForConnectionSignal.WaitForSignal(timeout) == ezThreadSignal::WaitResult::Timeout)
    {
      return EZ_FAILURE;
    }
  }

  return m_pChannel->IsConnected() ? EZ_SUCCESS : EZ_FAILURE;
}

bool ezProcessCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}
