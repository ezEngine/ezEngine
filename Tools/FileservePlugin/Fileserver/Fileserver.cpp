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

  bool bSuccess = false;
  ezUInt32 uiFileSize = 0;

  ezStringBuilder sFoundPathRel, sFoundPathAbs;
  ezFileserveClientContext::DataDir* pDataDir = nullptr;
  if (FindFileInDataDirs(client, sRequestedFile, sFoundPathRel, sFoundPathAbs, &pDataDir).Succeeded())
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
        bSuccess = true;
        ezLog::Success("Sent {0} bytes of '{1}'", uiSentFileSize, sRequestedFile);
      }
    }
  }

  {
    if (bSuccess)
    {
      //ezLog::Info("Finished sending '{0}'", sRequestedFile);
    }
    else
    {
      ezLog::Info("File does not exist '{0}'", sRequestedFile);
    }

    const ezUInt16 uiEndToken = 0;

    ezNetworkMessage ret('FSRV', 'FILE');
    ret.GetWriter() << uiDownloadID;
    ret.GetWriter() << uiEndToken;
    ret.GetWriter() << uiFileSize;
    ret.GetWriter() << bSuccess;

    if (bSuccess)
    {
      ret.GetWriter() << pDataDir->m_sMountPoint;
      ret.GetWriter() << sFoundPathRel;
    }

    m_Network->Send(ezNetworkTransmitMode::Reliable, ret);
  }
}

ezResult ezFileserver::FindFileInDataDirs(ezFileserveClientContext &client, const char* szRequestedFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, ezFileserveClientContext::DataDir** ppDataDir)
{
  for (ezUInt32 i = client.m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    auto& dd = client.m_MountedDataDirs[i - 1];

    const char* szSubPathToUse = szRequestedFile;

    if (szRequestedFile[0] == ':') // a rooted path
    {
      ezLog::Warning("Path is rooted: {0}", szRequestedFile);

      // skip all data dirs that do not have the same root name
      // dd.m_sRootName already ends with a / to prevent incorrect matches
      if (!ezStringUtils::StartsWith(szRequestedFile, dd.m_sRootName))
        continue;

      szSubPathToUse = szRequestedFile + dd.m_sRootName.GetElementCount();
      ezLog::Warning("Found data dir for rooted path: '{0}' -> '{1}'", szRequestedFile, szSubPathToUse);
    }
    else if (ezStringUtils::StartsWith(szRequestedFile, dd.m_sPathOnClient))
    {
      szSubPathToUse = szRequestedFile + dd.m_sPathOnClient.GetElementCount();

      ezLog::Warning("Found file with client prefix: '{0}' | '{1}'", dd.m_sPathOnClient, szSubPathToUse);
    }

    out_sAbsPath = dd.m_sPathOnServer;
    out_sAbsPath.AppendPath(szSubPathToUse);

    if (ezOSFile::ExistsFile(out_sAbsPath))
    {
      out_sRelPath = szSubPathToUse;

      if (ppDataDir)
      {
        *ppDataDir = &dd;
      }

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

