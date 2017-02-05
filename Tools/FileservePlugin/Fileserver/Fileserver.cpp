#include <PCH.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>

EZ_IMPLEMENT_SINGLETON(ezFileserver);

ezFileserver::ezFileserver()
  : m_SingletonRegistrar(this)
{
}

void ezFileserver::StartServer()
{
  m_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);
  m_Network->StartServer('EZFS', 1042, false);
  m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserver::NetworkMsgHandler, this));
}

void ezFileserver::StopServer()
{
  if (!m_Network)
    return;

  m_Network->ShutdownConnection();
  m_Network.Reset();
}

void ezFileserver::UpdateServer()
{
  if (!m_Network)
    return;

  m_Network->UpdateNetwork();
  m_Network->ExecuteAllMessageHandlers();
}

void ezFileserver::NetworkMsgHandler(ezNetworkMessage& msg)
{
  auto& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();
    ezLog::Info("Connected to new client with ID {0}", msg.GetApplicationID());
  }

  ezStringBuilder tmp;

  if (msg.GetMessageID() == 'MNT')
  {
    ezNetworkMessage ret;

    msg.GetReader() >> tmp;
    ezLog::Info(" Mount: '{0}'", tmp);

    ret.SetMessageID('FSRV', 'RMNT');
    m_Network->Send(ezNetworkTransmitMode::Reliable, ret);

    client.m_MountedDataDirs.PushBack(tmp);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    msg.GetReader() >> tmp;
    ezLog::Info(" Read: '{0}'", tmp);

    ezUInt16 uiDownloadID;
    msg.GetReader() >> uiDownloadID;

    ezFileReader file;
    if (file.Open(tmp).Succeeded())
    {
      const ezUInt64 uiFileSize = file.GetFileSize();

      ezUInt8 chunk[1024];

      while (true)
      {
        const ezUInt64 uiRead = file.ReadBytes(chunk, 1024);

        ezLog::Info("Sending {0} bytes of '{1}'", uiRead, tmp);

        ezNetworkMessage ret;
        ret.GetWriter() << uiDownloadID;
        ret.GetWriter() << (ezUInt16)uiRead;
        ret.GetWriter().WriteBytes(chunk, uiRead);

        ret.SetMessageID('FSRV', 'FILE');
        m_Network->Send(ezNetworkTransmitMode::Reliable, ret);

        if (uiRead < 1024)
          break;
      }
    }

    {
      ezLog::Info("Finished sending '{0}'", tmp);

      ezNetworkMessage ret('FSRV', 'FILE');
      ret.GetWriter() << 0;
      m_Network->Send(ezNetworkTransmitMode::Reliable, ret);
    }

    return;
  }

  ezLog::Warning("FSRV: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

