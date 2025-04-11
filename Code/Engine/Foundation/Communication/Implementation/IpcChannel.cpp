#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>

#if EZ_ENABLED(EZ_SUPPORTS_IPC)
#  include <PipeChannel_Platform.h>
#endif

static_assert((ezInt32)ezIpcChannel::ConnectionState::Disconnected == (ezInt32)ezIpcChannelEvent::Disconnected);
static_assert((ezInt32)ezIpcChannel::ConnectionState::Connecting == (ezInt32)ezIpcChannelEvent::Connecting);
static_assert((ezInt32)ezIpcChannel::ConnectionState::Connected == (ezInt32)ezIpcChannelEvent::Connected);

ezIpcChannel::ezIpcChannel(ezStringView sAddress, Mode::Enum mode)
  : m_sAddress(sAddress)
  , m_Mode(mode)
  , m_pOwner(ezMessageLoop::GetSingleton())
{
}

ezIpcChannel::~ezIpcChannel()
{
  m_pOwner->RemoveChannel(this);
}

ezInternal::NewInstance<ezIpcChannel> ezIpcChannel::CreatePipeChannel(ezStringView sAddress, Mode::Enum mode)
{
  EZ_IGNORE_UNUSED(sAddress);
  EZ_IGNORE_UNUSED(mode);

  if (sAddress.IsEmpty() || sAddress.GetElementCount() > 200)
  {
    ezLog::Error("Failed co create pipe '{0}', name is not valid", sAddress);
    return nullptr;
  }

#if EZ_ENABLED(EZ_SUPPORTS_IPC)
  return EZ_DEFAULT_NEW(ezPipeChannel_Platform, sAddress, mode);
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


ezInternal::NewInstance<ezIpcChannel> ezIpcChannel::CreateNetworkChannel(ezStringView sAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return EZ_DEFAULT_NEW(ezIpcChannelEnet, sAddress, mode);
#else
  EZ_IGNORE_UNUSED(sAddress);
  EZ_IGNORE_UNUSED(mode);
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


bool ezIpcChannel::Send(ezArrayPtr<const ezUInt8> data)
{
  {
    EZ_LOCK(m_OutputQueueMutex);
    ezMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    ezMemoryStreamWriter writer(&storage);
    ezUInt32 uiSize = data.GetCount() + HEADER_SIZE;
    ezUInt32 uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    EZ_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    writer.WriteBytes(data.GetPtr(), data.GetCount()).AssertSuccess("Failed to write to in-memory buffer, out of memory?");
  }
  if (IsConnected())
  {
    EZ_LOCK(m_pOwner->m_TasksMutex);
    if (!m_pOwner->m_SendQueue.Contains(this))
      m_pOwner->m_SendQueue.PushBack(this);
    if (NeedWakeup())
    {
      m_pOwner->WakeUp();
    }
    return true;
  }
  return false;
}

void ezIpcChannel::SetReceiveCallback(ReceiveCallback callback)
{
  EZ_LOCK(m_ReceiveCallbackMutex);
  m_ReceiveCallback = callback;
}

ezResult ezIpcChannel::WaitForMessages(ezTime timeout)
{
  if (IsConnected())
  {
    if (timeout == ezTime::MakeZero())
    {
      m_IncomingMessages.WaitForSignal();
    }
    else if (m_IncomingMessages.WaitForSignal(timeout) == ezThreadSignal::WaitResult::Timeout)
    {
      return EZ_FAILURE;
    }
  }
  return EZ_SUCCESS;
}

void ezIpcChannel::SetConnectionState(ezEnum<ezIpcChannel::ConnectionState> state)
{
  const ezEnum<ezIpcChannel::ConnectionState> oldValue = m_ConnectionState.Set(state);

  if (state != oldValue)
  {
    m_Events.Broadcast(ezIpcChannelEvent((ezIpcChannelEvent::Type)state.GetValue(), this));
  }
}

void ezIpcChannel::ReceiveData(ezArrayPtr<const ezUInt8> data)
{
  EZ_LOCK(m_ReceiveCallbackMutex);

  if (!m_ReceiveCallback.IsValid())
  {
    m_MessageAccumulator.PushBackRange(data);
    return;
  }

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
      m_ReceiveCallback(ezArrayPtr<const ezUInt8>(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE));
      m_IncomingMessages.RaiseSignal();
      m_Events.Broadcast(ezIpcChannelEvent(ezIpcChannelEvent::NewMessages, this));
      m_MessageAccumulator.Clear();
    }
  }
}

void ezIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}
