#include <Foundation/PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <ThirdParty/enet/enet.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>

class ezTelemetryThread;

ezTelemetry::ezEventTelemetry ezTelemetry::s_TelemetryEvents;
ezUInt32 ezTelemetry::s_uiApplicationID = 0;
ezUInt32 ezTelemetry::s_uiServerID = 0;
static bool g_bServerRunning = false;
bool ezTelemetry::s_bConnectedToServer = false;
bool ezTelemetry::s_bConnectedToClient = false;
ezTime ezTelemetry::s_PingToServer;
ezHybridString<32, ezStaticAllocatorWrapper> ezTelemetry::s_ServerName;
ezHybridString<32, ezStaticAllocatorWrapper> ezTelemetry::s_ServerIP;
static bool g_bInitialized = false;

static ENetAddress g_pServerAddress;
static ENetHost* g_pHost = NULL;
static ENetPeer* g_pConnectionToServer = NULL;
ezTelemetry::ConnectionMode ezTelemetry::s_ConnectionMode = ezTelemetry::None;
ezMap<ezUInt64, ezTelemetry::MessageQueue, ezCompareHelper<ezUInt64>, ezStaticAllocatorWrapper > ezTelemetry::s_SystemMessages;


void ezTelemetry::UpdateServerPing()
{
  enet_peer_ping(g_pConnectionToServer);
  ezTelemetry::s_PingToServer = ezTime::MilliSeconds(g_pConnectionToServer->lastRoundTripTime);
}

void ezTelemetry::UpdateNetwork()
{
  if (!g_pHost)
    return;

  static bool bRecursive = false;
  if (bRecursive)
    return;

  bRecursive = true;

  ENetEvent NetworkEvent;

  while (true) 
  {
    ezLock<ezMutex> Lock(GetTelemetryMutex());

    const ezInt32 iStatus = enet_host_service(g_pHost, &NetworkEvent, 0);

    if (iStatus <= 0)
    {
      bRecursive = false;
      return;
    }

    switch(NetworkEvent.type) 
    {
    case ENET_EVENT_TYPE_CONNECT:
      {
        if ((ezTelemetry::s_ConnectionMode == ezTelemetry::Server) && (NetworkEvent.peer->eventData != 'EZBC'))
        {
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        char szHostIP[64] = "";
        char szHostName[64] = "";

        enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);
        enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

        if (s_ConnectionMode == Client)
        {
          ezTelemetry::s_ServerName = szHostName;
          ezTelemetry::s_ServerIP   = szHostIP;
          s_bConnectedToServer = true;

          SendToServer('EZBC', 'RQID', NULL, 0);

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::ConnectedToServer;

          s_TelemetryEvents.Broadcast(e);
        }
        else
        {
          Broadcast(ezTelemetry::Reliable, 'EZBC', 'EZID', &s_uiApplicationID, sizeof(ezUInt32));
          s_bConnectedToClient = true;

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::ConnectedToClient;

          s_TelemetryEvents.Broadcast(e);
        }

        FlushOutgoingQueues();

        //ezLog::Info("New Connection from Host: '%s', IP: %s", szHostName, szHostIP);
      }
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (s_ConnectionMode == Client)
        {
          s_bConnectedToServer = false;

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          ezThreadUtils::Sleep(1000);

          // Now try to reconnect. If the Server still exists, fine, connect to that.
          // If it does not exist anymore, this will connect to the next best Server that can be found.
          g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'EZBC');

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromServer;

          s_TelemetryEvents.Broadcast(e);

        }
        else
        {
          /// \todo This assumes we only connect to a single client ...
          s_bConnectedToClient = false;

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromClient;

          s_TelemetryEvents.Broadcast(e);
        }

      }
      break;

    case ENET_EVENT_TYPE_RECEIVE:
      {
        const ezUInt64 uiSystemID = *((ezUInt64*) &NetworkEvent.packet->data[0]);
        const ezUInt32 uiMsgID    = *((ezUInt32*) &NetworkEvent.packet->data[8]);
        const ezUInt8* pData = &NetworkEvent.packet->data[12];

        if (uiSystemID == 'EZBC')
        {
          switch (uiMsgID)
          {
          case 'RQID':
            Broadcast(ezTelemetry::Reliable, 'EZBC', 'EZID', &s_uiApplicationID, sizeof(ezUInt32));
            break;

          case 'EZID':
            {
              s_uiServerID = *((ezUInt32*) pData);

              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ChangedServerID;

              s_TelemetryEvents.Broadcast(e);
            }
            break;
          }
        }
        else
        {
          MessageQueue& Queue = s_SystemMessages[uiSystemID];
          
          if (Queue.m_bAcceptMessages)
          {
            Queue.m_IncomingQueue.PushBack();
            ezTelemetryMessage& Msg = Queue.m_IncomingQueue.PeekBack();
            
            Msg.SetMessageID(uiSystemID, uiMsgID);
            Msg.GetWriter().WriteBytes(pData, NetworkEvent.packet->dataLength - 8);
          }
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

    }
  }

  bRecursive = false;
}

ezResult ezTelemetry::RetrieveMessage(ezUInt64 uiSystemID, ezTelemetryMessage& out_Message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return EZ_FAILURE;

  ezLock<ezMutex> Lock(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return EZ_FAILURE;

  out_Message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return EZ_SUCCESS;
}

void ezTelemetry::InitializeAsServer()
{
  g_pServerAddress.host = ENET_HOST_ANY;
  g_pServerAddress.port = s_uiPort;

  g_pHost = enet_host_create(&g_pServerAddress, 32, 2, 0, 0);

}

ezResult ezTelemetry::InitializeAsClient(const char* szConnectTo)
{
  g_pHost = enet_host_create(NULL, 1, 2, 0, 0);

  if (ezStringUtils::IsNullOrEmpty(szConnectTo) || ezStringUtils::IsEqual_NoCase(szConnectTo, "localhost"))
    enet_address_set_host(&g_pServerAddress, "localhost");
  else
  if (ezStringUtils::FindSubString(szConnectTo, ".") != NULL)
  {
    ezHybridArray<ezString, 8> IP;
    ezStringBuilder s = szConnectTo;
    s.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return EZ_FAILURE;

    const ezUInt32 ip1 = atoi(IP[0].GetData()) & 0xFF;
    const ezUInt32 ip2 = atoi(IP[1].GetData()) & 0xFF;
    const ezUInt32 ip3 = atoi(IP[2].GetData()) & 0xFF;
    const ezUInt32 ip4 = atoi(IP[3].GetData()) & 0xFF;

    const ezUInt32 uiIP = (ip1 | ip2 << 8 | ip3 << 16 | ip4 << 24);

    g_pServerAddress.host = uiIP;
  }
  else
    enet_address_set_host(&g_pServerAddress, szConnectTo);

  g_pServerAddress.port = s_uiPort;

  g_pConnectionToServer = NULL;
  g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'EZBC');

  if (g_pConnectionToServer)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezResult ezTelemetry::OpenConnection(ConnectionMode Mode, const char* szConnectTo)
{
  CloseConnection();

  if (!g_bInitialized)
  {
    enet_initialize();
    g_bInitialized = true;
  }

  s_uiApplicationID = (ezUInt32) ezSystemTime::Now().GetSeconds();

  switch (Mode)
  {
  case ezTelemetry::Server:
    InitializeAsServer();
    break;
  case ezTelemetry::Client:
    if (InitializeAsClient(szConnectTo) == EZ_FAILURE)
    {
      CloseConnection();
      return EZ_FAILURE;
    }
    break;
  }

  s_ConnectionMode = Mode;

  ezTelemetry::UpdateNetwork();

  StartTelemetryThread();

  return EZ_SUCCESS;
}

void ezTelemetry::Transmit(TransmitMode tm, const void* pData, ezUInt32 uiDataBytes)
{
  if (!g_pHost)
    return;

  ezLock<ezMutex> Lock(GetTelemetryMutex());

  ENetPacket* pPacket = enet_packet_create(pData, uiDataBytes, (tm == Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(g_pHost, 0, pPacket);

  // make sure the message is processed immediately
  ezTelemetry::UpdateNetwork();
}

void ezTelemetry::Send(TransmitMode tm, ezUInt64 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
  if (!g_pHost)
    return;

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
  else
  {
    // when we do have a connection, just send the message out

    ezHybridArray<ezUInt8, 64> TempData;
    TempData.SetCount(12 + uiDataBytes);
    *((ezUInt64*) &TempData[0]) = uiSystemID;
    *((ezUInt32*) &TempData[8]) = uiMsgID;

    if (pData && uiDataBytes > 0)
      ezMemoryUtils::Copy((ezUInt8*) &TempData[12], (ezUInt8*) pData, uiDataBytes);

    Transmit(tm, &TempData[0], TempData.GetCount());
  }
}

void ezTelemetry::Send(TransmitMode tm, ezUInt64 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes)
{
  if (!g_pHost)
    return;

  EZ_ASSERT(iDataBytes != 0, "Use the non-stream Send() method when you want to send messages without extra data.");

  const ezUInt32 uiStackSize = 1024;

  ezHybridArray<ezUInt8, uiStackSize + 8> TempData;
  TempData.SetCount(12);
  *((ezUInt64*) &TempData[0]) = uiSystemID;
  *((ezUInt32*) &TempData[8]) = uiMsgID;

  // if we don't know how much to take out of the stream, read the data piece by piece from the input stream
  if (iDataBytes < 0)
  {
    while (true)
    {
      const ezUInt32 uiOffset = TempData.GetCount();
      TempData.SetCount(uiOffset + uiStackSize); // no allocation the first time
      
      const ezUInt32 uiRead = static_cast<ezUInt32>(Stream.ReadBytes(&TempData[uiOffset], uiStackSize));

      if (uiRead < uiStackSize)
      {
        // resize the array down to its actual size
        TempData.SetCount(uiOffset + uiRead);
        break;
      }
    }

  }
  else
  {
    TempData.SetCount(12 + iDataBytes);
    Stream.ReadBytes(&TempData[12], iDataBytes);
  }

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, &TempData[12], TempData.GetCount() - 12);
  else
  {
    // when we do have a connection, just send the message out
    Transmit(tm, &TempData[0], TempData.GetCount());
  }
}

void ezTelemetry::CloseConnection()
{
  s_bConnectedToServer = false;
  s_bConnectedToClient = false;
  s_ConnectionMode = None;
  s_uiServerID = 0;
  g_pConnectionToServer = NULL;

  StopTelemetryThread();

  // prevent other threads from interfering
  ezLock<ezMutex> Lock(GetTelemetryMutex());

  UpdateNetwork();
  ezThreadUtils::Sleep(10);

  if (g_pHost)
  {
    // send all peers that we are disconnecting
    for (ezUInt32 i = (ezUInt32) g_pHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&g_pHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    ezThreadUtils::Sleep(10);
  }

  // finally close the network connection
  if (g_pHost)
  {
    enet_host_destroy(g_pHost);
    g_pHost = NULL;
  }

  if (g_bInitialized)
  {
    enet_deinitialize();
    g_bInitialized = false;
  }


    // if there are any queued messages, throw them away
  for (ezMap<ezUInt64, ezTelemetry::MessageQueue, ezCompareHelper<ezUInt64>, ezStaticAllocatorWrapper >::Iterator it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_IncomingQueue.Clear();
    it.Value().m_OutgoingQueue.Clear();
  }

}





