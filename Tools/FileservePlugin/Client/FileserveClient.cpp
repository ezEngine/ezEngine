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
  m_sServerConnectionAddress = "localhost:1042";
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

    m_sFileserveCacheFolder = ezOSFile::GetUserDataFolder("ezFileserve/Cache");
    m_sFileserveCacheMetaFolder = ezOSFile::GetUserDataFolder("ezFileserve/Meta");

    if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
    {
      ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
      return EZ_FAILURE;
    }

    if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheMetaFolder).Failed())
    {
      ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheMetaFolder);
      return EZ_FAILURE;
    }

    if (m_Network->ConnectToServer('EZFS', m_sServerConnectionAddress).Failed())
      return EZ_FAILURE;

    if (m_Network->WaitForConnectionToServer(ezTime::Seconds(3)).Failed())
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


void ezFileserveClient::GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const
{
  ezStringBuilder sMountPoint;
  GetDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(sMountPoint);
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
    ezInt8 iFileStatus = 0;
    msg.GetReader() >> iFileStatus;

    // iFileStatus == 0: file does not exist on server
    // iFileStatus == 1: file was not downloaded, because it was already up to date on the client

    if (iFileStatus == 2) // request successful
    {
      ezInt64 iFileTimeStamp = 0;
      ezUInt64 uiFileHash = 0;

      ezStringBuilder sMountPoint, sFoundPathRel;
      msg.GetReader() >> sMountPoint;
      msg.GetReader() >> sFoundPathRel;
      msg.GetReader() >> iFileTimeStamp;
      msg.GetReader() >> uiFileHash;

      if (uiFileSize != m_Download.GetCount())
      {
        ezLog::Error("Transfered file size ({0} bytes) does not match expected size ({1} bytes)", m_Download.GetCount(), uiFileSize);
      }
      else
      {
        ezLog::Dev("Finished download: {0} bytes for '{1}'", m_Download.GetCount(), m_sCurrentFileDownload);

        ezStringBuilder sCachedFile, sCachedMetaFile;
        CreateFileserveCachePath(sFoundPathRel, sMountPoint, sCachedFile, sCachedMetaFile);

        // write the downloaded file to our cache directory
        {
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

        // Write the meta file
        {
          ezOSFile file;
          if (file.Open(sCachedMetaFile, ezFileMode::Write).Succeeded())
          {
            file.Write(&iFileTimeStamp, sizeof(ezInt64));
            file.Write(&uiFileHash, sizeof(ezUInt64));

            file.Close();
          }
          else
          {
            ezLog::Error("Failed to write meta file to '{0}'", sCachedMetaFile);
          }
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

  ezInt64 iKnownTimestamp = 0;
  ezUInt64 uiKnownHash = 0;

  IsFileCached(szFile, out_sRelPath, out_sAbsPath, &iKnownTimestamp, &uiKnownHash);

  m_Download.Clear();

  m_sCurrentFileDownload = szFile;
  m_bDownloading = true;
  ++m_uiCurFileDownload;

  ezNetworkMessage msg('FSRV', 'READ');
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_uiCurFileDownload;
  msg.GetWriter() << iKnownTimestamp;
  msg.GetWriter() << uiKnownHash;

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

bool ezFileserveClient::IsFileCached(const char* szFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, ezInt64* out_pTimeStamp, ezUInt64* out_pFileHash) const
{
  if (out_pTimeStamp)
    *out_pTimeStamp = 0;
  if (out_pFileHash)
    *out_pFileHash = 0;

  ezStringBuilder sMetaFile;

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
    {
      sMetaFile = m_sFileserveCacheMetaFolder;
      sMetaFile.AppendPath(dd.m_sMountPoint, szRelSubPath);
      sMetaFile.MakeCleanPath();

      ezOSFile meta;
      if (meta.Open(sMetaFile, ezFileMode::Read).Failed())
        return false;

      if (out_pTimeStamp && out_pFileHash)
      {
        meta.Read(out_pTimeStamp, sizeof(ezInt64));
        meta.Read(out_pFileHash, sizeof(ezUInt64));
      }

      return true;
    }
  }

  return false;
}

void ezFileserveClient::CreateFileserveCachePath(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sFullPathMeta) const
{
  {
    out_sAbsPath = m_sFileserveCacheFolder;
    out_sAbsPath.AppendPath(szMountPoint);

    if (szFile[1] == ':')
      out_sAbsPath.AppendPath(&szFile[3]);
    else
      out_sAbsPath.AppendPath(szFile);

    out_sAbsPath.MakeCleanPath();
  }

  {
    out_sFullPathMeta = m_sFileserveCacheMetaFolder;
    out_sFullPathMeta.AppendPath(szMountPoint);

    if (szFile[1] == ':')
      out_sFullPathMeta.AppendPath(&szFile[3]);
    else
      out_sFullPathMeta.AppendPath(szFile);

    out_sFullPathMeta.MakeCleanPath();
  }
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}
