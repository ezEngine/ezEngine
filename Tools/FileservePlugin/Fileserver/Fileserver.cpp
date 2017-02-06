#include <PCH.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Algorithm/Hashing.h>

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

bool ezFileserver::UpdateServer()
{
  if (!m_Network)
    return false;

  m_Network->UpdateNetwork();
  return m_Network->ExecuteAllMessageHandlers() > 0;
}

void ezFileserver::NetworkMsgHandler(ezNetworkMessage& msg)
{
  auto& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();
    ezLog::Info("Connected to new client with ID {0}", msg.GetApplicationID());
  }

  if (msg.GetMessageID() == 'MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  ezLog::Warning("FSRV: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

void ezFileserver::HandleMountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezStringBuilder sDataDir, sRootName, sMountPoint;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;

  ezLog::Info(" Mounting: '{0}', RootName = '{1}', MountPoint = '{2}'", sDataDir, sRootName, sMountPoint);

  if (!sRootName.IsEmpty() && !sRootName.EndsWith("/"))
  {
    sRootName.Append("/");
  }

  auto& dir = client.m_MountedDataDirs.ExpandAndGetRef();
  dir.m_sPathOnClient = sDataDir;
  dir.m_sPathOnServer = sDataDir; /// \todo Redirect path
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;
}

void ezFileserver::HandleFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  ezUInt16 uiDownloadID;
  msg.GetReader() >> uiDownloadID;

  ezInt64 iClientTimestamp = 0;
  ezUInt64 uiClientHash = 0;
  msg.GetReader() >> iClientTimestamp;
  msg.GetReader() >> uiClientHash;

  ezInt64 iServerTimestamp = -1; // client will send 0, if not existing, make sure the values are different
  ezUInt64 uiServerHash = 0;

  ezInt8 iSuccess = 0;
  ezUInt32 uiFileSize = 0;

  ezStringBuilder sFoundPathRel, sFoundPathAbs;
  const ezFileserveClientContext::DataDir* pDataDir = nullptr;
  if (client.FindFileInDataDirs(sRequestedFile, sFoundPathRel, sFoundPathAbs, &pDataDir).Succeeded())
  {
    ezFileStats stat;
    if (ezOSFile::GetFileStats(sFoundPathAbs, stat).Succeeded())
    {
      iServerTimestamp = stat.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Microsecond);
    }

    if (iServerTimestamp == iClientTimestamp)
    {
      iSuccess = 1;
      ezLog::Info("File unchanged on client: '{0}'", sRequestedFile);
    }
    else
    {
      ezFileReader file;
      if (file.Open(sFoundPathAbs).Succeeded())
      {
        uiFileSize = (ezUInt32)file.GetFileSize();
        ezUInt64 uiSentFileSize = 0;

        ezUInt8 chunk[1024];

        while (true)
        {
          const ezUInt64 uiRead = file.ReadBytes(chunk, 1024);

          if (uiRead == 0)
            break;

          uiServerHash = ezHashing::MurmurHash64(chunk, uiRead, uiServerHash);

          ezNetworkMessage ret;
          ret.GetWriter() << uiDownloadID;
          ret.GetWriter() << (ezUInt16)uiRead;
          ret.GetWriter() << uiFileSize;
          ret.GetWriter().WriteBytes(chunk, uiRead);

          ret.SetMessageID('FSRV', 'FILE');
          m_Network->Send(ezNetworkTransmitMode::Reliable, ret);

          uiSentFileSize += uiRead;

          if (uiRead < 1024)
            break;
        }

        if (uiSentFileSize == uiFileSize)
        {
          iSuccess = 2;
          ezLog::Success("Sent file to client: '{0}'", sRequestedFile);
        }
      }
    }
  }

  {
    if (iSuccess > 0)
    {
      //ezLog::Info("Finished sending '{0}'", sRequestedFile);
    }
    else
    {
      //ezLog::Info("File does not exist '{0}'", sRequestedFile);
    }

    const ezUInt16 uiEndToken = 0;

    ezNetworkMessage ret('FSRV', 'FILE');
    ret.GetWriter() << uiDownloadID;
    ret.GetWriter() << uiEndToken;
    ret.GetWriter() << uiFileSize;
    ret.GetWriter() << iSuccess;

    if (iSuccess == 2)
    {
      ret.GetWriter() << pDataDir->m_sMountPoint;
      ret.GetWriter() << sFoundPathRel;
      ret.GetWriter() << iServerTimestamp;
      ret.GetWriter() << uiServerHash;
    }

    m_Network->Send(ezNetworkTransmitMode::Reliable, ret);
  }
}

