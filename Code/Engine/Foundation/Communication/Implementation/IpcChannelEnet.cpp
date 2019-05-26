#include <FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>

ezIpcChannelEnet::ezIpcChannelEnet(const char* szAddress, Mode::Enum mode)
    : ezIpcChannel(szAddress, mode)
{
  m_sAddress = szAddress;
  m_Network = ezRemoteInterfaceEnet::Make();
  m_Network->SetMessageHandler(0, ezMakeDelegate(&ezIpcChannelEnet::NetworkMessageHandler, this));
  m_Network->m_RemoteEvents.AddEventHandler(ezMakeDelegate(&ezIpcChannelEnet::EnetEventHandler, this));

  m_pOwner->AddChannel(this);
}

ezIpcChannelEnet::~ezIpcChannelEnet()
{
  m_Network->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void ezIpcChannelEnet::InternalConnect()
{
  if (m_Mode == Mode::Server)
  {
    m_Network->StartServer('RMOT', m_sAddress, false);
  }
  else
  {
    if ((m_sLastAddress != m_sAddress) || (ezTime::Now() - m_LastConnectAttempt > ezTime::Seconds(10)))
    {
      m_sLastAddress = m_sAddress;
      m_LastConnectAttempt = ezTime::Now();
      m_Network->ConnectToServer('RMOT', m_sAddress, false);
    }

    m_Network->WaitForConnectionToServer(ezTime::Milliseconds(10.0));
  }

  m_Connected = m_Network->IsConnectedToOther() ? 1 : 0;
}

void ezIpcChannelEnet::InternalDisconnect()
{
  m_Network->ShutdownConnection();
  m_Network->m_RemoteEvents.RemoveEventHandler(ezMakeDelegate(&ezIpcChannelEnet::EnetEventHandler, this));

  m_Connected = 0;
}

void ezIpcChannelEnet::InternalSend()
{
  {
    EZ_LOCK(m_OutputQueueMutex);

    while (!m_OutputQueue.IsEmpty())
    {
      ezMemoryStreamStorage& storage = m_OutputQueue.PeekFront();

      m_Network->Send(ezRemoteTransmitMode::Reliable, 0, 0, storage.GetData(), storage.GetStorageSize());

      m_OutputQueue.PopFront();
    }
  }

  m_Network->UpdateRemoteInterface();
}

bool ezIpcChannelEnet::NeedWakeup() const
{
  return true;
}

void ezIpcChannelEnet::Tick()
{
  m_Network->UpdateRemoteInterface();

  m_Connected = m_Network->IsConnectedToOther() ? 1 : 0;

  m_Network->ExecuteAllMessageHandlers();
}

void ezIpcChannelEnet::NetworkMessageHandler(ezRemoteMessage& msg)
{
  ezArrayPtr<const ezUInt8> data(msg.GetMessageData(), msg.GetMessageSize());

  ReceiveMessageData(data);
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



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannelEnet);

