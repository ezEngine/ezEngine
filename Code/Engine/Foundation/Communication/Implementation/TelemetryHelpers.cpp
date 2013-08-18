#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

void ezTelemetry::QueueOutgoingMessage(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
  // unreliable packages can just be dropped
  if (tm == ezTelemetry::Unreliable)
    return;

  ezLock<ezMutex> Lock(GetTelemetryMutex());

  // add a new message to the queue
  MessageQueue& Queue = s_SystemMessages[uiSystemID];
  Queue.m_OutgoingQueue.PushBack();

  // and fill it out properly
  ezTelemetryMessage& msg = Queue.m_OutgoingQueue.PeekBack();
  msg.SetMessageID(uiSystemID, uiMsgID);

  if (uiDataBytes > 0)
    msg.GetWriter().WriteBytes(pData, uiDataBytes);

  // if our outgoing queue has grown too large, dismis older messages
  if (Queue.m_OutgoingQueue.GetCount() > Queue.m_uiMaxQueuedOutgoing)
    Queue.m_OutgoingQueue.PopFront(Queue.m_OutgoingQueue.GetCount() - Queue.m_uiMaxQueuedOutgoing);
}

void ezTelemetry::FlushOutgoingQueues()
{
  // if there is no connection to anyone (yet), don't do anything
  if (!IsConnectedToOther())
    return;

  ezLock<ezMutex> Lock(GetTelemetryMutex());

  // go through all system types
  for (ezMap<ezUInt32, ezTelemetry::MessageQueue, ezCompareHelper<ezUInt32>, ezStaticAllocatorWrapper >::Iterator it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_OutgoingQueue.IsEmpty())
      continue;

    const ezUInt32 uiCurCount = it.Value().m_OutgoingQueue.GetCount(); 

    // send all messages that are queued for this system
    for (ezUInt32 i = 0; i < uiCurCount; ++i)
      Send(ezTelemetry::Reliable, it.Value().m_OutgoingQueue[i]); // Send() will already update the network

    // check that they have not been queue again
    EZ_ASSERT(it.Value().m_OutgoingQueue.GetCount() == uiCurCount, "Implementation Error: When queued messages are flushed, they should not get queued again.");

    it.Value().m_OutgoingQueue.Clear();
  }
}


ezResult ezTelemetry::ConnectToServer(const char* szConnectTo)
{
  return OpenConnection(Client, szConnectTo);
}

ezResult ezTelemetry::CreateServer()
{
  return OpenConnection(Server);
}

void ezTelemetry::AcceptMessagesForSystem(ezUInt32 uiSystemID, bool bAccept)
{
  s_SystemMessages[uiSystemID].m_bAcceptMessages = bAccept;
}

void ezTelemetry::SetOutgoingQueueSize(ezUInt32 uiSystemID, ezUInt16 uiMaxQueued)
{
  s_SystemMessages[uiSystemID].m_uiMaxQueuedOutgoing = uiMaxQueued;
}


bool ezTelemetry::IsConnectedToOther()
{
  return ((s_ConnectionMode == Client && IsConnectedToServer()) || (s_ConnectionMode == Server && IsConnectedToClient()));
}

void ezTelemetry::Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
  if (s_ConnectionMode != ezTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void ezTelemetry::Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes)
{
  if (s_ConnectionMode != ezTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, Stream, iDataBytes);
}

void ezTelemetry::Broadcast(TransmitMode tm, ezTelemetryMessage& Msg)
{
  if (s_ConnectionMode != ezTelemetry::Server)
    return;

  Send(tm, Msg);
}

void ezTelemetry::SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
  if (s_ConnectionMode != ezTelemetry::Client)
    return;

  Send(ezTelemetry::Reliable, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void ezTelemetry::SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes)
{
  if (s_ConnectionMode != ezTelemetry::Client)
    return;

  Send(ezTelemetry::Reliable, uiSystemID, uiMsgID, Stream, iDataBytes);
}

void ezTelemetry::SendToServer(ezTelemetryMessage& Msg)
{
  if (s_ConnectionMode != ezTelemetry::Client)
    return;

  Send(ezTelemetry::Reliable, Msg);
}

void ezTelemetry::Send(TransmitMode tm, ezTelemetryMessage& msg)
{
  Send(tm, msg.GetSystemID(), msg.GetMessageID(), msg.GetReader(), msg.m_Storage.GetStorageSize());
}


namespace ezLogWriter
{
  void NetworkBroadcast::LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('LOG', 'MSG');

    msg.GetWriter() << (ezUInt16) EventData.m_EventType;
    msg.GetWriter() << (ezUInt16) EventData.m_uiIndentation;
    msg.GetWriter() << EventData.m_szTag;
    msg.GetWriter() << EventData.m_szText;
    //ezUInt32 len = ezStringUtils::GetStringElementCount(EventData.m_szText) + 1;
    //msg.GetWriter().WriteBytes(&len, 4);
    //msg.GetWriter().WriteBytes(EventData.m_szText, len);

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }
}