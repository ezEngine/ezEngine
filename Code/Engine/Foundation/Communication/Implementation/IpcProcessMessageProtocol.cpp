#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
//#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
//#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

ezIpcProcessMessageProtocol::ezIpcProcessMessageProtocol(ezIpcChannel* pChannel)
{
  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(ezMakeDelegate(&ezIpcProcessMessageProtocol::ReceiveMessageData, this));
}

ezIpcProcessMessageProtocol::~ezIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  ezDeque<ezUniquePtr<ezProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();
}

bool ezIpcProcessMessageProtocol::Send(ezProcessMessage* pMsg)
{
  ezContiguousMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);
  return m_pChannel->Send(ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool ezIpcProcessMessageProtocol::ProcessMessages()
{
  ezDeque<ezUniquePtr<ezProcessMessage>> messages;
  SwapWorkQueue(messages);
  if (messages.IsEmpty())
  {
    return false;
  }

  while (!messages.IsEmpty())
  {
    ezUniquePtr<ezProcessMessage> msg = std::move(messages.PeekFront());
    messages.PopFront();
    m_MessageEvent.Broadcast(msg.Borrow());
  }

  return true;
}

ezResult ezIpcProcessMessageProtocol::WaitForMessages(ezTime timeout)
{
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

void ezIpcProcessMessageProtocol::SwapWorkQueue(ezDeque<ezUniquePtr<ezProcessMessage>>& messages)
{
  EZ_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  EZ_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return;
  messages.Swap(m_IncomingQueue);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcProcessMessageProtocol);
