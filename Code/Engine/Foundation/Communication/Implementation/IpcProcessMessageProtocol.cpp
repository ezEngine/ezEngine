#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
// #include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
// #include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Tracing/TraceProvider.h>

ezIpcProcessMessageProtocol::ezIpcProcessMessageProtocol(ezIpcChannel* pChannel)
{
  ezStringView sAddress = pChannel->GetAddress();

  m_uiReceiveChannelId = ezHashingUtils::xxHash64String(sAddress);
  m_uiSendChannelId = ezHashingUtils::xxHash64String(sAddress, 1337);

  if (pChannel->GetMode() == ezIpcChannel::Mode::Client)
  {
    // To make sure telemetry events can be correlated, the client swaps the stable channel IDs.
    std::swap(m_uiReceiveChannelId, m_uiSendChannelId);
  }

  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(ezMakeDelegate(&ezIpcProcessMessageProtocol::ReceiveMessageData, this));
}

ezIpcProcessMessageProtocol::~ezIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  while (ezUniquePtr<ezProcessMessage> msg = PopMessage())
  {
  }
}

bool ezIpcProcessMessageProtocol::Send(ezProcessMessage* pMsg)
{
  ezStringBuilder sTypeName = pMsg->GetDynamicRTTI()->GetTypeName();
  ezUInt64 uiMessageId = (ezUInt64)m_iSendMessageId.Increment();
  [[maybe_unused]] ezUInt64 uiRequestId = m_uiSendChannelId + uiMessageId;
  pMsg->m_uiMessageId = uiMessageId;

  ezContiguousMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);

  EZ_TRACE_ASYNC_BEGIN("IpcProtocol_Send", uiRequestId, ezTraceLevel::Info,
    EZ_TRACE_VALUE("Address", m_pChannel->GetAddress().GetStartPointer()), // Safe as underlying storage is ezString
    EZ_TRACE_VALUE("MessageId", uiMessageId),
    EZ_TRACE_VALUE("Type", sTypeName.GetData()));

  return m_pChannel->Send(ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool ezIpcProcessMessageProtocol::ProcessMessages()
{
  bool messagesPresent = false;

  while (ezUniquePtr<ezProcessMessage> msg = PopMessage())
  {
    [[maybe_unused]] ezUInt64 uiRequestId = m_uiReceiveChannelId + msg->m_uiMessageId;
    ezStringBuilder sTypeName = msg->GetDynamicRTTI()->GetTypeName();
    EZ_TRACE_ASYNC_END("IpcProtocol_Send", uiRequestId);

    EZ_TRACE_EVENT("IpcProtocol_Receive", ezTraceLevel::Info,
      EZ_TRACE_VALUE("Address", m_pChannel->GetAddress().GetStartPointer()), // Safe as underlying storage is ezString
      EZ_TRACE_VALUE("MessageId", msg->m_uiMessageId),
      EZ_TRACE_VALUE("Type", sTypeName.GetData()));
    messagesPresent = true;
    Event e;
    e.m_pMessage = msg.Borrow();
    e.m_bInterruptMessageProcessing = false;
    m_MessageEvent.Broadcast(e);
    if (e.m_bInterruptMessageProcessing)
      break;
  }

  return messagesPresent;
}

ezResult ezIpcProcessMessageProtocol::WaitForMessages(ezTime timeout)
{
  // Message processing can be interrupted via the m_bInterruptMessageProcessing flag. Thus, there is no guarantee that the queue is empty at this point. Only wait if the queue is empty.
  if (ProcessMessages())
  {
    return EZ_SUCCESS;
  }

  ezResult res = m_pChannel->WaitForMessages(timeout);
  if (res.Succeeded())
  {
    ProcessMessages();
  }
  return res;
}

void ezIpcProcessMessageProtocol::ReceiveMessageData(ezArrayPtr<const ezUInt8> data)
{
  // Message complete, de-serialize
  ezRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  const ezRTTI* pRtti = nullptr;

  ezProcessMessage* pMsg = (ezProcessMessage*)ezReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
  ezUniquePtr<ezProcessMessage> msg(pMsg, ezFoundation::GetDefaultAllocator());
  if (msg != nullptr)
  {
    EnqueueMessage(std::move(msg));
  }
  else
  {
    ezLog::Error("Channel received invalid Message!");
  }
}

void ezIpcProcessMessageProtocol::EnqueueMessage(ezUniquePtr<ezProcessMessage>&& msg)
{
  EZ_LOCK(m_IncomingQueueMutex);
  m_IncomingQueue.PushBack(std::move(msg));
}

ezUniquePtr<ezProcessMessage> ezIpcProcessMessageProtocol::PopMessage()
{
  EZ_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return {};

  ezUniquePtr<ezProcessMessage> front = std::move(m_IncomingQueue.PeekFront());
  m_IncomingQueue.PopFront();
  return front;
}
