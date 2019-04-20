#include <FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <enet/enet.h>

class ezTelemetryThread;

ezTelemetry::ezEventTelemetry ezTelemetry::s_TelemetryEvents;
ezUInt32 ezTelemetry::s_uiApplicationID = 0;
ezUInt32 ezTelemetry::s_uiServerID = 0;
ezUInt16 ezTelemetry::s_uiPort = 1040;
bool ezTelemetry::s_bConnectedToServer = false;
bool ezTelemetry::s_bConnectedToClient = false;
bool ezTelemetry::s_bAllowNetworkUpdate = true;
ezTime ezTelemetry::s_PingToServer;
ezString ezTelemetry::s_ServerName;
ezString ezTelemetry::s_ServerIP;
static bool g_bInitialized = false;
ezTelemetry::ConnectionMode ezTelemetry::s_ConnectionMode = ezTelemetry::None;
ezMap<ezUInt64, ezTelemetry::MessageQueue> ezTelemetry::s_SystemMessages;

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
static ENetAddress g_pServerAddress;
static ENetHost* g_pHost = nullptr;
static ENetPeer* g_pConnectionToServer = nullptr;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

void ezTelemetry::UpdateServerPing()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  enet_peer_ping(g_pConnectionToServer);
  ezTelemetry::s_PingToServer = ezTime::Milliseconds(g_pConnectionToServer->lastRoundTripTime);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::UpdateNetwork()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  if (!s_bAllowNetworkUpdate)
    return;

  s_bAllowNetworkUpdate = false;

  ENetEvent NetworkEvent;

  while (true)
  {
    EZ_LOCK(GetTelemetryMutex());

    const ezInt32 iStatus = enet_host_service(g_pHost, &NetworkEvent, 0);

    if (iStatus <= 0)
    {
      s_bAllowNetworkUpdate = true;
      return;
    }

    switch (NetworkEvent.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        if ((ezTelemetry::s_ConnectionMode == ezTelemetry::Server) && (NetworkEvent.peer->eventData != 'EZBC'))
        {
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        if (s_ConnectionMode == Client)
        {
          char szHostIP[64] = "<unknown>";
          char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);

          // Querying host IP and name can take a lot of time which can lead to timeouts
          // enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          ezTelemetry::s_ServerIP = szHostIP;
          // ezTelemetry::s_ServerName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          // got a new client, send the server ID to it
          s_bConnectedToClient = true; // we need this fake state, otherwise Broadcast will queue the message instead of sending it
          Broadcast(ezTelemetry::Reliable, 'EZBC', 'EZID', &s_uiApplicationID, sizeof(ezUInt32));
          s_bConnectedToClient = false;

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (s_ConnectionMode == Client)
        {
          s_bConnectedToServer = false;

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          ezThreadUtils::Sleep(ezTime::Seconds(1));

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
        const ezUInt32 uiSystemID = *((ezUInt32*)&NetworkEvent.packet->data[0]);
        const ezUInt32 uiMsgID = *((ezUInt32*)&NetworkEvent.packet->data[4]);
        const ezUInt8* pData = &NetworkEvent.packet->data[8];

        if (uiSystemID == 'EZBC')
        {
          switch (uiMsgID)
          {
            case 'EZID':
            {
              s_uiServerID = *((ezUInt32*)pData);

              // connection to server is finalized
              s_bConnectedToServer = true;

              // acknowledge that the ID has been received
              SendToServer('EZBC', 'AKID', nullptr, 0);

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToServer;

              s_TelemetryEvents.Broadcast(e);

              FlushOutgoingQueues();
            }
            break;
            case 'AKID':
            {
              // the client received the server ID -> the connection has been established properly

              /// \todo This assumes we only connect to a single client ...
              s_bConnectedToClient = true;

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToClient;

              s_TelemetryEvents.Broadcast(e);

              SendServerName();
              FlushOutgoingQueues();
            }
            break;

            case 'NAME':
            {
              s_ServerName = reinterpret_cast<const char*>(pData);
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

            EZ_ASSERT_DEV(
              (ezUInt32)NetworkEvent.packet->dataLength >= 8, "Message Length Invalid: {0}", (ezUInt32)NetworkEvent.packet->dataLength);

            Msg.GetWriter().WriteBytes(pData, NetworkEvent.packet->dataLength - 8);
          }
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

      default:
        break;
    }
  }

  s_bAllowNetworkUpdate = true;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::SetServerName(const char* name)
{
  if (s_ConnectionMode == ConnectionMode::Client)
    return;

  if (s_ServerName == name)
    return;

  s_ServerName = name;

  SendServerName();
}

void ezTelemetry::SendServerName()
{
  if (!IsConnectedToOther())
    return;

  char data[48];
  ezStringUtils::Copy(data, EZ_ARRAY_SIZE(data), s_ServerName.GetData());

  Broadcast(ezTelemetry::Reliable, 'EZBC', 'NAME', data, EZ_ARRAY_SIZE(data));
}

ezResult ezTelemetry::RetrieveMessage(ezUInt32 uiSystemID, ezTelemetryMessage& out_Message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return EZ_FAILURE;

  EZ_LOCK(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return EZ_FAILURE;

  out_Message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return EZ_SUCCESS;
}

void ezTelemetry::InitializeAsServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pServerAddress.host = ENET_HOST_ANY;
  g_pServerAddress.port = s_uiPort;

  g_pHost = enet_host_create(&g_pServerAddress, 32, 2, 0, 0);
#else
  ezLog::SeriousWarning("Enet is not compiled into this build, ezTelemetry::InitializeAsServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

ezResult ezTelemetry::InitializeAsClient(const char* szConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pHost = enet_host_create(nullptr, 1, 2, 0, 0);

  ezStringBuilder sConnectTo = szConnectTo;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, ezStringUtils::GetStringElementCount(szColon));

    ezStringBuilder sPort = szColon + 1;
    s_uiPort = static_cast<ezUInt16>(atoi(sPort.GetData()));
  }

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
    enet_address_set_host(&g_pServerAddress, "localhost");
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    ezHybridArray<ezString, 8> IP;
    sConnectTo.Split(false, IP, ".");

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
    enet_address_set_host(&g_pServerAddress, sConnectTo.GetData());

  g_pServerAddress.port = s_uiPort;

  g_pConnectionToServer = nullptr;
  g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'EZBC');

  if (g_pConnectionToServer)
    return EZ_SUCCESS;
#else
  ezLog::SeriousWarning("Enet is not compiled into this build, ezTelemetry::InitializeAsClient() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

  return EZ_FAILURE;
}

ezResult ezTelemetry::OpenConnection(ConnectionMode Mode, const char* szConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  CloseConnection();

  if (!g_bInitialized)
  {
    EZ_VERIFY(enet_initialize() == 0, "Enet could not be initialized.");
    g_bInitialized = true;
  }

  s_uiApplicationID = (ezUInt32)ezTime::Now().GetSeconds();

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
    default:
      break;
  }

  s_ConnectionMode = Mode;

  ezTelemetry::UpdateNetwork();

  StartTelemetryThread();

  return EZ_SUCCESS;
#else
  ezLog::SeriousWarning("Enet is not compiled into this build, ezTelemetry::OpenConnection() will be ignored.");
  return EZ_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::Transmit(TransmitMode tm, const void* pData, ezUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  EZ_LOCK(GetTelemetryMutex());

  ENetPacket* pPacket = enet_packet_create(pData, uiDataBytes, (tm == Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(g_pHost, 0, pPacket);

  // make sure the message is processed immediately
  ezTelemetry::UpdateNetwork();
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
  else
  {
    // when we do have a connection, just send the message out

    ezHybridArray<ezUInt8, 64> TempData;
    TempData.SetCountUninitialized(8 + uiDataBytes);
    *((ezUInt32*)&TempData[0]) = uiSystemID;
    *((ezUInt32*)&TempData[4]) = uiMsgID;

    if (pData && uiDataBytes > 0)
      ezMemoryUtils::Copy((ezUInt8*)&TempData[8], (ezUInt8*)pData, uiDataBytes);

    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReader& Stream, ezInt32 iDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  const ezUInt32 uiStackSize = 1024;

  ezHybridArray<ezUInt8, uiStackSize + 8> TempData;
  TempData.SetCountUninitialized(8);
  *((ezUInt32*)&TempData[0]) = uiSystemID;
  *((ezUInt32*)&TempData[4]) = uiMsgID;

  // if we don't know how much to take out of the stream, read the data piece by piece from the input stream
  if (iDataBytes < 0)
  {
    while (true)
    {
      const ezUInt32 uiOffset = TempData.GetCount();
      TempData.SetCountUninitialized(uiOffset + uiStackSize); // no allocation the first time

      const ezUInt32 uiRead = static_cast<ezUInt32>(Stream.ReadBytes(&TempData[uiOffset], uiStackSize));

      if (uiRead < uiStackSize)
      {
        // resize the array down to its actual size
        TempData.SetCountUninitialized(uiOffset + uiRead);
        break;
      }
    }
  }
  else
  {
    TempData.SetCountUninitialized(8 + iDataBytes);

    if (iDataBytes > 0)
      Stream.ReadBytes(&TempData[8], iDataBytes);
  }

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
  {
    if (TempData.GetCount() > 8)
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, &TempData[8], TempData.GetCount() - 8);
    else
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, nullptr, 0);
  }
  else
  {
    // when we do have a connection, just send the message out
    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void ezTelemetry::CloseConnection()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  s_bConnectedToServer = false;
  s_bConnectedToClient = false;
  s_ConnectionMode = None;
  s_uiServerID = 0;
  g_pConnectionToServer = nullptr;

  StopTelemetryThread();

  // prevent other threads from interfering
  EZ_LOCK(GetTelemetryMutex());

  UpdateNetwork();
  ezThreadUtils::Sleep(ezTime::Milliseconds(10));

  if (g_pHost)
  {
    // send all peers that we are disconnecting
    for (ezUInt32 i = (ezUInt32)g_pHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&g_pHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    ezThreadUtils::Sleep(ezTime::Milliseconds(10));
  }

  // finally close the network connection
  if (g_pHost)
  {
    enet_host_destroy(g_pHost);
    g_pHost = nullptr;
  }

  if (g_bInitialized)
  {
    enet_deinitialize();
    g_bInitialized = false;
  }

  // if there are any queued messages, throw them away
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_IncomingQueue.Clear();
    it.Value().m_OutgoingQueue.Clear();
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Telemetry);
