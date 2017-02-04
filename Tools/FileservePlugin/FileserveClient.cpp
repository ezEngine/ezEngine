#include <PCH.h>
#include <FileservePlugin/FileserveClient.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Communication/GlobalEvent.h>

EZ_IMPLEMENT_SINGLETON(ezFileserveClient);

ezFileserveClient::ezFileserveClient()
  : m_SingletonRegistrar(this)
{
}

ezFileserveClient::~ezFileserveClient()
{
  ezLog::Info("Shutting down fileserve client");

  if (m_Network)
  {
    m_Network->ShutdownConnection();
    m_Network.Reset();
  }
}

void ezFileserveClient::EnsureConnected()
{
  if (m_Network == nullptr)
  {
    m_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);
    m_Network->ConnectToServer('EZFS', "localhost:1042");

    ezUInt32 uiMaxRounds = 100;
    while (!m_Network->IsConnectedToServer() && uiMaxRounds > 0)
    {
      --uiMaxRounds;
      m_Network->UpdateNetwork();
      ezThreadUtils::Sleep(100);
    }

    if (uiMaxRounds == 0)
      ezLog::Error("Connection to ezFileserver timed out");
    else
    {
      ezLog::Info("Connected to ezFileserver");

      m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));
    }
  }
}

void ezFileserveClient::UpdateClient()
{
  if (!m_Network->IsConnectedToServer())
    return;

  m_Network->ExecuteAllMessageHandlers();
}

ezDataDirectoryType* ezFileserveClient::MountDataDirectory(const char* szDataDir)
{
  if (!m_Network->IsConnectedToServer())
    return nullptr;

  ezNetworkMessage msg('FSRV', 'MNT');
  msg.GetWriter() << szDataDir;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);
  return nullptr;
}

void ezFileserveClient::NetworkMsgHandler(ezNetworkMessage& msg)
{
  ezLog::Info("FSRV: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}
