#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/RemoteInterfaceEnet.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>

ezIpcChannelEnet::ezIpcChannelEnet(ezStringView sAddress, Mode::Enum mode)
  : ezIpcChannel(sAddress, mode)
  , m_sAddress(sAddress)
{
  m_pNetwork = ezRemoteInterfaceEnet::Make();
  m_pNetwork->SetMessageHandler(0, ezMakeDelegate(&ezIpcChannelEnet::NetworkMessageHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(ezMakeDelegate(&ezIpcChannelEnet::EnetEventHandler, this));

  m_pOwner->AddChannel(this);
}

ezIpcChannelEnet::~ezIpcChannelEnet()
{
  m_pNetwork->m_RemoteEvents.RemoveEventHandler(ezMakeDelegate(&ezIpcChannelEnet::EnetEventHandler, this));
  m_pNetwork->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void ezIpcChannelEnet::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Connecting)
    return;

  if (m_Mode == Mode::Server)
  {
    if (m_pNetwork->StartServer('RMOT', m_sAddress, false).Failed())
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
  }
  else
  {
    if (m_pNetwork->ConnectToServer('RMOT', m_sAddress, false).Failed())
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
    m_LastConnectAttempt = ezTime::Now();
  }
}

void ezIpcChannelEnet::InternalDisconnect()
{
  m_pNetwork->ShutdownConnection();
  SetConnectionState(ConnectionState::Disconnected);
}

void ezIpcChannelEnet::InternalSend()
{
  {
    EZ_LOCK(m_OutputQueueMutex);

    while (!m_OutputQueue.IsEmpty())
    {
      ezContiguousMemoryStreamStorage& storage = m_OutputQueue.PeekFront();

      m_pNetwork->Send(ezRemoteTransmitMode::Reliable, 0, 0, storage);

      m_OutputQueue.PopFront();
    }
  }

  m_pNetwork->UpdateRemoteInterface();
}

bool ezIpcChannelEnet::NeedWakeup() const
{
  return true;
}

void ezIpcChannelEnet::Tick()
{
  m_pNetwork->UpdateRemoteInterface();

  if (GetConnectionState() == ConnectionState::Connecting)
  {
    if (m_pNetwork->IsConnectedToOther())
    {
      m_LastConnectAttempt = ezTime::MakeZero();
      SetConnectionState(ConnectionState::Connected);
    }
    else if (m_Mode == Mode::Client && !m_LastConnectAttempt.IsZero() && ezTime::Now() - m_LastConnectAttempt > ezTime::MakeFromSeconds(2))
    {
      m_LastConnectAttempt = ezTime::MakeZero();
      SetConnectionState(ConnectionState::Disconnected);
    }
  }

  if (!m_pNetwork->IsConnectedToOther())
  {
    if (GetConnectionState() == ConnectionState::Connected)
    {
      SetConnectionState(ConnectionState::Disconnected);
    }
  }
  m_pNetwork->ExecuteAllMessageHandlers();
}

void ezIpcChannelEnet::NetworkMessageHandler(ezRemoteMessage& msg)
{
  ReceiveData(msg.GetMessageData());
}

void ezIpcChannelEnet::EnetEventHandler(const ezRemoteEvent& e)
{
  if (e.m_Type == ezRemoteEvent::ConnectedToClient)
  {
    SetConnectionState(ConnectionState::Connected);
  }

  if (e.m_Type == ezRemoteEvent::DisconnectedFromServer)
  {
    Disconnect();
  }

  if (e.m_Type == ezRemoteEvent::DisconnectedFromClient)
  {
    SetConnectionState(ConnectionState::Disconnected);
  }

  if (e.m_Type == ezRemoteEvent::ConnectedToServer)
  {
    m_LastConnectAttempt = ezTime::MakeZero();
    SetConnectionState(ConnectionState::Connected);
  }
}

#endif
