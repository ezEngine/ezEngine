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
  m_pNetwork->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void ezIpcChannelEnet::InternalConnect()
{
  if (m_Mode == Mode::Server)
  {
    m_pNetwork->StartServer('RMOT', m_sAddress, false).IgnoreResult();
    SetConnectionState(ConnectionState::Connecting);
  }
  else
  {
    SetConnectionState(ConnectionState::Connecting);
    if ((m_sLastAddress != m_sAddress) || (ezTime::Now() - m_LastConnectAttempt > ezTime::MakeFromSeconds(10)))
    {
      m_sLastAddress = m_sAddress;
      m_LastConnectAttempt = ezTime::Now();
      m_pNetwork->ConnectToServer('RMOT', m_sAddress, false).IgnoreResult();
    }

    m_pNetwork->WaitForConnectionToServer(ezTime::MakeFromMilliseconds(10.0)).IgnoreResult();
  }

  SetConnectionState(m_pNetwork->IsConnectedToOther() ? ConnectionState::Connected : ConnectionState::Disconnected);
}

void ezIpcChannelEnet::InternalDisconnect()
{
  m_pNetwork->ShutdownConnection();
  m_pNetwork->m_RemoteEvents.RemoveEventHandler(ezMakeDelegate(&ezIpcChannelEnet::EnetEventHandler, this));

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

  SetConnectionState(m_pNetwork->IsConnectedToOther() ? ConnectionState::Connected : ConnectionState::Disconnected);

  m_pNetwork->ExecuteAllMessageHandlers();
}

void ezIpcChannelEnet::NetworkMessageHandler(ezRemoteMessage& msg)
{
  ReceiveData(msg.GetMessageData());
}

void ezIpcChannelEnet::EnetEventHandler(const ezRemoteEvent& e)
{
  if (e.m_Type == ezRemoteEvent::DisconnectedFromServer)
  {
    ezLog::Info("Disconnected from remote engine process.");
    Disconnect();
  }

  if (e.m_Type == ezRemoteEvent::ConnectedToServer)
  {
    ezLog::Info("Connected to remote engine process.");
  }
}

#endif


