#include <PCH.h>
#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

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

ezResult ezFileserveClient::EnsureConnected()
{
  if (m_bFailedToConnect)
    return EZ_FAILURE;

  if (m_Network == nullptr)
  {
    m_bFailedToConnect = true;
    m_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);

    m_sFileserveCacheFolder = ezOSFile::GetUserDataFolder("FileserveCache");

    if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
    {
      ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
      return EZ_FAILURE;
    }

    {
      ezStringBuilder dummy(m_sFileserveCacheFolder, "/Dummy.txt");
      ezOSFile file;
      if (file.Open(dummy, ezFileMode::Write).Succeeded())
      {
        file.Close();
        ezOSFile::DeleteFile(dummy);
      }
      else
      {
        ezLog::Error("Could not write to dummy file");
        return EZ_FAILURE;
      }
    }

    if (m_Network->ConnectToServer('EZFS', "localhost:1042").Failed())
      return EZ_FAILURE;

    ezUInt32 uiMaxRounds = 20;
    while (!m_Network->IsConnectedToServer() && uiMaxRounds > 0)
    {
      --uiMaxRounds;
      m_Network->UpdateNetwork();
      ezThreadUtils::Sleep(100);
    }

    if (uiMaxRounds == 0)
    {
      m_Network->ShutdownConnection();
      ezLog::Error("Connection to ezFileserver timed out");
      return EZ_FAILURE;
    }
    else
    {
      ezLog::Info("Connected to ezFileserver");
      m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));
    }

    m_bFailedToConnect = false;
  }

  return EZ_SUCCESS;
}

void ezFileserveClient::UpdateClient()
{
  if (m_Network == nullptr || m_bFailedToConnect)
    return;

  if (!m_Network->IsConnectedToServer())
    return;

  m_Network->ExecuteAllMessageHandlers();
}

void ezFileserveClient::MountDataDirectory(const char* szDataDirectory, const char* szRootName)
{
  if (!m_Network->IsConnectedToServer())
    return;

  ezStringBuilder sDataDirPath = szDataDirectory;
  sDataDirPath.MakeCleanPath();
  if (!sDataDirPath.IsEmpty() && !sDataDirPath.EndsWith("/"))
    sDataDirPath.Append("/");

  ezStringBuilder sMountPoint;
  GetDataDirMountPoint(sDataDirPath, sMountPoint);

  ezNetworkMessage msg('FSRV', 'MNT');
  msg.GetWriter() << sDataDirPath;
  msg.GetWriter() << szRootName;
  msg.GetWriter() << sMountPoint;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);

  ezStringBuilder sRoot = szRootName;
  if (!sRoot.IsEmpty() && !sRoot.EndsWith("/"))
    sRoot.Append("/");

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  dd.m_sPathOnClient = sDataDirPath;
  dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
}


void ezFileserveClient::GetDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const
{
  EZ_ASSERT_DEV(ezStringUtils::IsNullOrEmpty(szDataDir) || ezStringUtils::EndsWith(szDataDir, "/"), "Invalid path");

  const ezUInt32 uiMountPoint = ezHashing::MurmurHash(szDataDir);
  out_sMountPoint.Format("{0}", ezArgU(uiMountPoint, 8, true, 16));
}


void ezFileserveClient::GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath) const
{
  ezStringBuilder sMountPoint;
  GetDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);
}

void ezFileserveClient::NetworkMsgHandler(ezNetworkMessage& msg)
{
  if (msg.GetMessageID() == 'FILE')
  {
    HandleFileTransferMsg(msg);
    return;
  }

  ezLog::Info("FSRV: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}


void ezFileserveClient::HandleFileTransferMsg(ezNetworkMessage &msg)
{
  ezUInt16 uiFileID = 0;
  msg.GetReader() >> uiFileID;

  if (uiFileID != m_uiCurFileDownload)
  {
    ezLog::Error("Fileserver is answering the wrong file request");
    return;
  }

  ezUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  ezUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  if (uiChunkSize > 0)
  {
    // make sure we don't need to reallocate
    m_Download.Reserve(uiFileSize);

    ezUInt32 uiStartPos = m_Download.GetCount();
    m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
    msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
  }
  else // download finished
  {
    bool bFileExistsOnServer = false;
    msg.GetReader() >> bFileExistsOnServer;


    if (bFileExistsOnServer)
    {
      ezStringBuilder sMountPoint, sFoundPathRel;
      msg.GetReader() >> sMountPoint;
      msg.GetReader() >> sFoundPathRel;

      if (uiFileSize != m_Download.GetCount())
      {
        ezLog::Error("Transfered file size ({0} bytes) does not match expected size ({1} bytes)", m_Download.GetCount(), uiFileSize);
      }
      else
      {
        ezLog::Dev("Finished download: {0} bytes for '{1}'", m_Download.GetCount(), m_sCurrentFileDownload);

        ezStringBuilder sCachedFile;
        CreateFileserveCachePath(sFoundPathRel, sMountPoint, sCachedFile);

        // write the downloaded file to our cache directory
        ezOSFile file;
        if (file.Open(sCachedFile, ezFileMode::Write).Succeeded())
        {
          if (!m_Download.IsEmpty())
            file.Write(m_Download.GetData(), m_Download.GetCount());

          file.Close();
        }
        else
        {
          ezLog::Error("Failed to write download to '{0}'", sCachedFile);
        }
      }
    }

    m_Download.Clear();
    m_bDownloading = false;
    return;
  }
}

ezResult ezFileserveClient::DownloadFile(const char* szFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath)
{
  if (!m_Network->IsConnectedToServer())
  {
    out_sRelPath = szFile;
    out_sAbsPath = szFile;
    return EZ_FAILURE;
  }

  if (IsFileCached(szFile, out_sRelPath, out_sAbsPath))
    return EZ_SUCCESS;

  m_Download.Clear();

  m_sCurrentFileDownload = szFile;
  m_bDownloading = true;
  ++m_uiCurFileDownload;

  ezNetworkMessage msg('FSRV', 'READ');
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_uiCurFileDownload;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_Network->UpdateNetwork();
    m_Network->ExecuteAllMessageHandlers();
  }

  if (IsFileCached(szFile, out_sRelPath, out_sAbsPath))
    return EZ_SUCCESS;

  out_sRelPath = szFile;
  out_sAbsPath = szFile;
  return EZ_FAILURE;
}

bool ezFileserveClient::IsFileCached(const char* szFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath) const
{
  for (ezUInt32 i = m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_MountedDataDirs[i - 1];

    const char* szRelSubPath = szFile;

    if (ezStringUtils::StartsWith(szFile, dd.m_sPathOnClient))
    {
      szRelSubPath = szFile + dd.m_sPathOnClient.GetElementCount();
    }

    if (szRelSubPath[1] == ':')
      szRelSubPath = &szFile[3];

    out_sRelPath = szRelSubPath;

    out_sAbsPath = m_sFileserveCacheFolder;
    out_sAbsPath.AppendPath(dd.m_sMountPoint, szRelSubPath);
    out_sAbsPath.MakeCleanPath();

    if (ezOSFile::ExistsFile(out_sAbsPath))
      return true;
  }

  return false;
}

void ezFileserveClient::CreateFileserveCachePath(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath) const
{
  out_sAbsPath = m_sFileserveCacheFolder;
  out_sAbsPath.AppendPath(szMountPoint);

  if (szFile[1] == ':')
    out_sAbsPath.AppendPath(&szFile[3]);
  else
    out_sAbsPath.AppendPath(szFile);

  out_sAbsPath.MakeCleanPath();
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}
