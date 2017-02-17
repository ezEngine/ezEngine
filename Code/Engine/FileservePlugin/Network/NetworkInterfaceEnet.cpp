#include <PCH.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>

bool ezNetworkInterfaceEnet::s_bEnetInitialized = false;

ezResult ezNetworkInterfaceEnet::InternalCreateConnection(ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress)
{
  if (!s_bEnetInitialized)
  {
    if (enet_initialize() != 0)
    {
      ezLog::Error("Failed to initialize Enet");
      return EZ_FAILURE;
    }

    s_bEnetInitialized = true;
  }

  m_pEnetConnectionToServer = nullptr;

  ENetAddress* pServerAddress = nullptr;
  size_t maxPeerCount = 1;
  const size_t maxChannels = 2;
  const enet_uint32 incomingBandwidth = 0; // unlimited
  const enet_uint32 outgoingBandwidth = 0; // unlimited

  if (mode == ezNetworkMode::Server)
  {
    m_EnetServerAddress.host = ENET_HOST_ANY;
    m_EnetServerAddress.port = uiPort;

    maxPeerCount = 8;
    pServerAddress = &m_EnetServerAddress;
  }
  else
  {
    if (DetermineTargetAddress(szServerAddress, m_EnetServerAddress.host, m_EnetServerAddress.port).Failed())
    {
      enet_address_set_host(&m_EnetServerAddress, szServerAddress);
    }

    // use default settings for enet_host_create
  }

  m_pEnetHost = enet_host_create(pServerAddress, maxPeerCount, maxChannels, incomingBandwidth, outgoingBandwidth);

  if (m_pEnetHost == nullptr)
  {
    ezLog::Error("Failed to create an Enet server");
    return EZ_FAILURE;
  }

  if (mode == ezNetworkMode::Client)
  {
    m_pEnetConnectionToServer = enet_host_connect(m_pEnetHost, &m_EnetServerAddress, maxChannels, GetConnectionToken());

    if (m_pEnetConnectionToServer == nullptr)
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezNetworkInterfaceEnet::InternalShutdownConnection()
{
  if (m_pEnetHost)
  {
    // send all peers that we are disconnecting
    for (ezUInt32 i = (ezUInt32)m_pEnetHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&m_pEnetHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    ezThreadUtils::Sleep(10);
  }

  // finally close the network connection
  if (m_pEnetHost)
  {
    enet_host_destroy(m_pEnetHost);
    m_pEnetHost = nullptr;
  }

  //enet_deinitialize();
  m_pEnetConnectionToServer = nullptr;
}

ezTime ezNetworkInterfaceEnet::InternalGetPingToServer()
{
  EZ_ASSERT_DEV(m_pEnetConnectionToServer != nullptr, "Client has not connected to server");

  enet_peer_ping(m_pEnetConnectionToServer);
  return ezTime::Milliseconds(m_pEnetConnectionToServer->lastRoundTripTime);
}

ezResult ezNetworkInterfaceEnet::InternalTransmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_pEnetHost == nullptr)
    return EZ_FAILURE;

  ENetPacket* pPacket = enet_packet_create(data.GetPtr(), data.GetCount(), (tm == ezNetworkTransmitMode::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(m_pEnetHost, 0, pPacket);

  return EZ_SUCCESS;
}

void ezNetworkInterfaceEnet::InternalUpdateNetwork()
{
  if (!m_pEnetHost)
    return;

  if (!m_bAllowNetworkUpdates)
    return;

  m_bAllowNetworkUpdates = false;
  EZ_SCOPE_EXIT(m_bAllowNetworkUpdates = true);

  ENetEvent NetworkEvent;

  while (true)
  {
    const ezInt32 iStatus = enet_host_service(m_pEnetHost, &NetworkEvent, 0);

    if (iStatus <= 0)
      return;

    switch (NetworkEvent.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      {
        if ((GetNetworkMode() == ezNetworkMode::Server) && (NetworkEvent.peer->eventData != GetConnectionToken()))
        {
          // do not accept connections that don't have the correct password
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        char szHostIP[64] = "";
        char szHostName[64] = "";

        enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);
        enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

        if (GetNetworkMode() == ezNetworkMode::Client)
        {
          m_ServerInfoName = szHostName;
          m_ServerInfoIP = szHostIP;

          // now we are waiting for the server to send its ID
        }
        else
        {
          const ezUInt32 uiAppID = GetApplicationID();
          Send(ezNetworkTransmitMode::Reliable, GetConnectionToken(), 'EZID', ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(&uiAppID), sizeof(ezUInt32)));

          // then wait for its acknowledgment message
        }
      }
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (GetNetworkMode() == ezNetworkMode::Client)
        {
          ezLog::Info("Disconnected from server, trying to reconnect");

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          ezThreadUtils::Sleep(1000);

          // Now try to reconnect. If the Server still exists, fine, connect to that.
          // If it does not exist anymore, this will connect to the next best Server that can be found.
          const size_t maxChannels = 2;
          m_pEnetConnectionToServer = enet_host_connect(m_pEnetHost, &m_EnetServerAddress, maxChannels, GetConnectionToken());

          ReportDisconnectedFromServer();
        }
        else
        {
          auto it = m_EnetPeerToClientID.Find(NetworkEvent.peer);
          if (it.IsValid())
          {
            ReportDisconnectedFromClient(it.Value());
            m_EnetPeerToClientID.Remove(it);
          }
        }
      }
      break;

    case ENET_EVENT_TYPE_RECEIVE:
      {
        const ezUInt32 uiApplicationID = *((ezUInt32*)&NetworkEvent.packet->data[0]);
        const ezUInt32 uiSystemID = *((ezUInt32*)&NetworkEvent.packet->data[4]);
        const ezUInt32 uiMsgID = *((ezUInt32*)&NetworkEvent.packet->data[8]);
        const ezUInt8* pData = &NetworkEvent.packet->data[12];

        if (uiSystemID == GetConnectionToken())
        {
          switch (uiMsgID)
          {
          case 'EZID':
            {
              // acknowledge that the ID has been received
              Send(GetConnectionToken(), 'AKID');

              // go tell the others about it
              ezUInt32 uiServerID = *((ezUInt32*)pData);
              ReportConnectionToServer(uiServerID);
            }
            break;

          case 'AKID':
            {
              m_EnetPeerToClientID[NetworkEvent.peer] = uiApplicationID;

              // the client received the server ID -> the connection has been established properly
              ReportConnectionToClient(uiApplicationID);
            }
            break;
          }
        }
        else
        {
          ReportMessage(uiApplicationID, uiSystemID, uiMsgID, ezArrayPtr<const ezUInt8>(pData, (ezUInt32)NetworkEvent.packet->dataLength - 12));
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;
    }
  }
}



EZ_STATICLINK_FILE(FileservePlugin, FileservePlugin_Network_NetworkInterfaceEnet);

