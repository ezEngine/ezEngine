#include <PCH.h>

#include <Foundation/Strings/FormatString.h>
#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

ezIpcChannel::ezIpcChannel(const char* szAddress, Mode::Enum mode)
    : m_Mode(mode), m_pOwner(ezMessageLoop::GetSingleton())
{
}

ezIpcChannel::~ezIpcChannel()
{
  ezDeque<ezUniquePtr<ezProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();

  m_pOwner->RemoveChannel(this);
}

ezIpcChannel* ezIpcChannel::CreatePipeChannel(const char* szAddress, Mode::Enum mode)
{
  if (ezStringUtils::IsNullOrEmpty(szAddress) || ezStringUtils::GetStringElementCount(szAddress) > 200)
  {
    ezLog::Error("Failed co create pipe '{0}', name is not valid", szAddress);
    return nullptr;
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  return EZ_DEFAULT_NEW(ezPipeChannel_win, szAddress, mode);
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


ezIpcChannel* ezIpcChannel::CreateNetworkChannel(const char* szAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return EZ_DEFAULT_NEW(ezIpcChannelEnet, szAddress, mode);
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}

void ezIpcChannel::Connect()
{
  EZ_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_ConnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


void ezIpcChannel::Disconnect()
{
  EZ_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_DisconnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}

bool ezIpcChannel::Send(ezProcessMessage* pMsg)
{
  {
    EZ_LOCK(m_OutputQueueMutex);
    ezMemoryStreamStorage& storage = m_OutputQueue.ExpandAndGetRef();
    ezMemoryStreamWriter writer(&storage);
    ezUInt32 iSize = 0;
    ezUInt32 iMagic = MAGIC_VALUE;
    writer << iMagic;
    writer << iSize;
    EZ_ASSERT_DEBUG(storage.GetStorageSize() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    ezReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);
    *reinterpret_cast<ezUInt32*>((ezUInt8*)storage.GetData() + 4) = storage.GetStorageSize();
  }
  if (m_Connected)
  {
    if (NeedWakeup())
    {
      EZ_LOCK(m_pOwner->m_TasksMutex);
      if (!m_pOwner->m_SendQueue.Contains(this))
        m_pOwner->m_SendQueue.PushBack(this);
      m_pOwner->WakeUp();
      return true;
    }
  }
  return false;
}

bool ezIpcChannel::ProcessMessages()
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

void ezIpcChannel::WaitForMessages()
{
  if (m_Connected)
  {
    m_IncomingMessages.WaitForSignal();
    ProcessMessages();
  }
}

void ezIpcChannel::ReceiveMessageData(ezArrayPtr<const ezUInt8> data)
{
  ezArrayPtr<const ezUInt8> remainingData = data;
  while (true)
  {
    if (m_MessageAccumulator.GetCount() < HEADER_SIZE)
    {
      if (remainingData.GetCount() + m_MessageAccumulator.GetCount() < HEADER_SIZE)
      {
        m_MessageAccumulator.PushBackRange(remainingData);
        return;
      }
      else
      {
        ezUInt32 uiRemainingHeaderData = HEADER_SIZE - m_MessageAccumulator.GetCount();
        ezArrayPtr<const ezUInt8> headerData = remainingData.GetSubArray(0, uiRemainingHeaderData);
        m_MessageAccumulator.PushBackRange(headerData);
        EZ_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == HEADER_SIZE, "We should have a full header now.");
        remainingData = remainingData.GetSubArray(uiRemainingHeaderData);
      }
    }

    EZ_ASSERT_DEBUG(m_MessageAccumulator.GetCount() >= HEADER_SIZE, "Header must be complete at this point.");
    if (remainingData.IsEmpty())
      return;

    // Read and verify header
    ezUInt32 uiMagic = *reinterpret_cast<const ezUInt32*>(m_MessageAccumulator.GetData());
    EZ_IGNORE_UNUSED(uiMagic);
    EZ_ASSERT_DEBUG(uiMagic == MAGIC_VALUE, "Message received with wrong magic value.");
    ezUInt32 uiMessageSize = *reinterpret_cast<const ezUInt32*>(m_MessageAccumulator.GetData() + 4);
    EZ_ASSERT_DEBUG(uiMessageSize < MAX_MESSAGE_SIZE, "Message too big: {0}! Either the stream got corrupted or you need to increase MAX_MESSAGE_SIZE.", uiMessageSize);
    if (uiMessageSize > remainingData.GetCount() + m_MessageAccumulator.GetCount())
    {
      m_MessageAccumulator.PushBackRange(remainingData);
      return;
    }

    // Write missing data into message accumulator
    ezUInt32 remainingMessageData = uiMessageSize - m_MessageAccumulator.GetCount();
    ezArrayPtr<const ezUInt8> messageData = remainingData.GetSubArray(0, remainingMessageData);
    m_MessageAccumulator.PushBackRange(messageData);
    EZ_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == uiMessageSize, "");
    remainingData = remainingData.GetSubArray(remainingMessageData);

    {
      // Message complete, de-serialize
      ezRawMemoryStreamReader reader(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE);
      const ezRTTI* pRtti = nullptr;

      ezProcessMessage* pMsg = (ezProcessMessage*)ezReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
      ezUniquePtr<ezProcessMessage> msg(pMsg, ezFoundation::GetDefaultAllocator());
      if (msg != NULL)
      {
        EnqueueMessage(std::move(msg));
      }
      else
      {
        ezLog::Error("Channel received invalid Message!");
      }
      m_MessageAccumulator.Clear();
    }
  }
}

void ezIpcChannel::EnqueueMessage(ezUniquePtr<ezProcessMessage>&& msg)
{
  {
    EZ_LOCK(m_IncomingQueueMutex);
    m_IncomingQueue.PushBack(std::move(msg));
  }
  m_IncomingMessages.RaiseSignal();

  m_Events.Broadcast(ezIpcChannelEvent(ezIpcChannelEvent::NewMessages, this));
}

void ezIpcChannel::SwapWorkQueue(ezDeque<ezUniquePtr<ezProcessMessage>>& messages)
{
  EZ_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  EZ_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return;
  messages.Swap(m_IncomingQueue);
}

void ezIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannel);
