#include <FoundationPCH.h>

#include <Foundation/Communication/RemoteInterfaceEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <enet/enet.h>

class ezRemoteInterfaceEnetImpl : public ezRemoteInterfaceEnet
{

protected:
  virtual void InternalUpdateRemoteInterface() override;
  virtual ezResult InternalCreateConnection(ezRemoteMode mode, const char* szServerAddress) override;
  virtual void InternalShutdownConnection() override;
  virtual ezTime InternalGetPingToServer() override;
  virtual ezResult InternalTransmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) override;

private:
  ENetAddress m_EnetServerAddress;
  ENetHost* m_pEnetHost = nullptr;
  ENetPeer* m_pEnetConnectionToServer = nullptr;
  bool m_bAllowNetworkUpdates = true;
  ezMap<void*, ezUInt32> m_EnetPeerToClientID;

  static bool s_bEnetInitialized;
};

ezInternal::NewInstance<ezRemoteInterfaceEnet> ezRemoteInterfaceEnet::Make(ezAllocatorBase* allocator /*= ezFoundation::GetDefaultAllocator()*/)
{
  return EZ_NEW(allocator, ezRemoteInterfaceEnetImpl);
}

ezRemoteInterfaceEnet::ezRemoteInterfaceEnet() = default;
ezRemoteInterfaceEnet::~ezRemoteInterfaceEnet() = default;

bool ezRemoteInterfaceEnetImpl::s_bEnetInitialized = false;

ezResult ezRemoteInterfaceEnetImpl::InternalCreateConnection(ezRemoteMode mode, const char* szServerAddress)
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

  {
    // Extract port from address
    const char* szPort = ezStringUtils::FindLastSubString(szServerAddress, ":");
    szPort = (szPort) ? szPort + 1 : szServerAddress;
    ezInt32 iPort = 0;
    if (ezConversionUtils::StringToInt(szPort, iPort).Failed())
    {
      ezLog::Error("Failed to extract port from server address: {0}", szServerAddress);
      return EZ_FAILURE;
    }
    m_uiPort = static_cast<ezUInt16>(iPort);
  }

  m_pEnetConnectionToServer = nullptr;

  ENetAddress* pServerAddress = nullptr;
  size_t maxPeerCount = 1;
  const size_t maxChannels = 2;
  const enet_uint32 incomingBandwidth = 0; // unlimited
  const enet_uint32 outgoingBandwidth = 0; // unlimited

  if (mode == ezRemoteMode::Server)
  {
    m_EnetServerAddress.host = ENET_HOST_ANY;
    m_EnetServerAddress.port = m_uiPort;

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

  if (mode == ezRemoteMode::Client)
  {
    m_pEnetConnectionToServer = enet_host_connect(m_pEnetHost, &m_EnetServerAddress, maxChannels, GetConnectionToken());

    if (m_pEnetConnectionToServer == nullptr)
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezRemoteInterfaceEnetImpl::InternalShutdownConnection()
{
  m_uiPort = 0;

  if (m_pEnetHost)
  {
    // send all peers that we are disconnecting
    for (ezUInt32 i = (ezUInt32)m_pEnetHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&m_pEnetHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateRemoteInterface();
    ezThreadUtils::Sleep(ezTime::Milliseconds(10));
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

ezTime ezRemoteInterfaceEnetImpl::InternalGetPingToServer()
{
  EZ_ASSERT_DEV(m_pEnetConnectionToServer != nullptr, "Client has not connected to server");

  enet_peer_ping(m_pEnetConnectionToServer);
  return ezTime::Milliseconds(m_pEnetConnectionToServer->lastRoundTripTime);
}

ezResult ezRemoteInterfaceEnetImpl::InternalTransmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data)
{
  if (m_pEnetHost == nullptr)
    return EZ_FAILURE;

  ENetPacket* pPacket = enet_packet_create(data.GetPtr(), data.GetCount(), (tm == ezRemoteTransmitMode::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(m_pEnetHost, 0, pPacket);

  return EZ_SUCCESS;
}

void ezRemoteInterfaceEnetImpl::InternalUpdateRemoteInterface()
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
        if ((GetRemoteMode() == ezRemoteMode::Server) && (NetworkEvent.peer->eventData != GetConnectionToken()))
        {
          // do not accept connections that don't have the correct password
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }


        if (GetRemoteMode() == ezRemoteMode::Client)
        {
          // Querying host IP and name can take a lot of time
          // Do not do this in the other case, as it may result in timeouts while establishing the connection.
          char szHostIP[64] = "<unknown>";
          //char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);
          //enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          m_ServerInfoIP = szHostIP;
          //m_ServerInfoName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          const ezUInt32 uiAppID = GetApplicationID();
          Send(ezRemoteTransmitMode::Reliable, GetConnectionToken(), 'EZID', ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(&uiAppID), sizeof(ezUInt32)));

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (GetRemoteMode() == ezRemoteMode::Client)
        {
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
              if (m_EnetPeerToClientID[NetworkEvent.peer] != uiApplicationID)
              {
                m_EnetPeerToClientID[NetworkEvent.peer] = uiApplicationID;

                // the client received the server ID -> the connection has been established properly
                ReportConnectionToClient(uiApplicationID);
              }
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

      default:
        break;
    }
  }
}

#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteInterfaceEnet);

