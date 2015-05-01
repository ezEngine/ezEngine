#include <Foundation/PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace
{
  ezProfilingId g_PerFrameUpdateProfilingId = ezProfilingSystem::CreateId("Telemetry.PerFrameUpdate");
}

void ezTelemetry::QueueOutgoingMessage(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
  // unreliable packages can just be dropped
  if (tm == ezTelemetry::Unreliable)
    return;

  EZ_LOCK(GetTelemetryMutex());

  // add a new message to the queue
  MessageQueue& Queue = s_SystemMessages[uiSystemID];
  Queue.m_OutgoingQueue.PushBack();

  // and fill it out properly
  ezTelemetryMessage& msg = Queue.m_OutgoingQueue.PeekBack();
  msg.SetMessageID(uiSystemID, uiMsgID);

  if (uiDataBytes > 0)
    msg.GetWriter().WriteBytes(pData, uiDataBytes);

  // if our outgoing queue has grown too large, dismiss older messages
  if (Queue.m_OutgoingQueue.GetCount() > Queue.m_uiMaxQueuedOutgoing)
    Queue.m_OutgoingQueue.PopFront(Queue.m_OutgoingQueue.GetCount() - Queue.m_uiMaxQueuedOutgoing);
}

void ezTelemetry::FlushOutgoingQueues()
{
  static bool bRecursion = false;

  if (bRecursion)
    return;

  // if there is no connection to anyone (yet), don't do anything
  if (!IsConnectedToOther())
    return;

  bRecursion = true;

  EZ_LOCK(GetTelemetryMutex());

  // go through all system types
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_OutgoingQueue.IsEmpty())
      continue;

    const ezUInt32 uiCurCount = it.Value().m_OutgoingQueue.GetCount();

    // send all messages that are queued for this system
    for (ezUInt32 i = 0; i < uiCurCount; ++i)
      Send(ezTelemetry::Reliable, it.Value().m_OutgoingQueue[i]); // Send() will already update the network

    // check that they have not been queue again
    EZ_ASSERT_DEV(it.Value().m_OutgoingQueue.GetCount() == uiCurCount, "Implementation Error: When queued messages are flushed, they should not get queued again.");

    it.Value().m_OutgoingQueue.Clear();
  }

  bRecursion = false;
}


ezResult ezTelemetry::ConnectToServer(const char* szConnectTo)
{
  return OpenConnection(Client, szConnectTo);
}

void ezTelemetry::CreateServer()
{
  EZ_VERIFY(OpenConnection(Server) == EZ_SUCCESS, "Opening a connection as a server should not be possible to fail.");
}

void ezTelemetry::AcceptMessagesForSystem(ezUInt32 uiSystemID, bool bAccept, ProcessMessagesCallback Callback, void* pPassThrough)
{
  EZ_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_bAcceptMessages = bAccept;
  s_SystemMessages[uiSystemID].m_Callback = Callback;
  s_SystemMessages[uiSystemID].m_pPassThrough = pPassThrough;
}

void ezTelemetry::PerFrameUpdate()
{
  EZ_PROFILE(g_PerFrameUpdateProfilingId);
  EZ_LOCK(GetTelemetryMutex());

  // Call each callback to process the incoming messages
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_IncomingQueue.IsEmpty() && it.Value().m_Callback)
      it.Value().m_Callback(it.Value().m_pPassThrough);
  }

  TelemetryEventData e;
  e.m_EventType = TelemetryEventData::PerFrameUpdate;

  const bool bAllowUpdate = s_bAllowNetworkUpdate;
  s_bAllowNetworkUpdate = false;
  s_TelemetryEvents.Broadcast(e);
  s_bAllowNetworkUpdate = bAllowUpdate;
}

void ezTelemetry::SetOutgoingQueueSize(ezUInt32 uiSystemID, ezUInt16 uiMaxQueued)
{
  EZ_LOCK(GetTelemetryMutex());

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

void ezTelemetry::Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReaderBase& Stream, ezInt32 iDataBytes)
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

void ezTelemetry::SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReaderBase& Stream, ezInt32 iDataBytes)
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




EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryHelpers);

