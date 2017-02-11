#include <PCH.h>
#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Types/ScopeExit.h>

EZ_IMPLEMENT_SINGLETON(ezFileserveClient);

bool ezFileserveClient::s_bEnableFileserve = true;

ezFileserveClient::ezFileserveClient()
  : m_SingletonRegistrar(this)
{
  m_sServerConnectionAddress = "localhost:1042";
}

ezFileserveClient::~ezFileserveClient()
{
  if (m_Network)
  {
    ezLog::Dev("Shutting down fileserve client");

    m_Network->ShutdownConnection();
    m_Network.Reset();
  }
}

ezResult ezFileserveClient::EnsureConnected()
{
  if (!s_bEnableFileserve || m_bFailedToConnect)
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
      ezLog::Success("Connected to ezFileserver");
      m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));
    }

    m_bFailedToConnect = false;
  }

  return EZ_SUCCESS;
}

void ezFileserveClient::UpdateClient()
{
  if (m_Network == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
    return;

  if (!m_Network->IsConnectedToServer())
    return;

  m_Network->ExecuteAllMessageHandlers();
}

void ezFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sFullPathMeta) const
{
  EZ_ASSERT_DEV(!ezPathUtils::IsAbsolutePath(szFile), "Invalid path");

  out_sAbsPath = m_sFileserveCacheFolder;
  out_sAbsPath.AppendPath(szMountPoint, szFile);
  out_sAbsPath.MakeCleanPath();

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(szMountPoint, szFile);
  out_sFullPathMeta.MakeCleanPath();
}

void ezFileserveClient::ComputeDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const
{
  EZ_ASSERT_DEV(ezStringUtils::IsNullOrEmpty(szDataDir) || ezStringUtils::EndsWith(szDataDir, "/"), "Invalid path");

  const ezUInt32 uiMountPoint = ezHashing::MurmurHash(szDataDir);
  out_sMountPoint.Format("{0}", ezArgU(uiMountPoint, 8, true, 16));
}

void ezFileserveClient::GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const
{
  ezStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDir, sMountPoint);

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

  if (msg.GetMessageID() == 'FINE')
  {
    HandleFileTransferFinishedMsg(msg);
    return;
  }

  ezLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

ezUInt16 ezFileserveClient::MountDataDirectory(const char* szDataDirectory, const char* szRootName)
{
  if (!m_Network->IsConnectedToServer())
    return 0xffff;

  ezStringBuilder sRoot = szRootName;
  sRoot.Trim(":/");

  ezStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDirectory, sMountPoint);

  const ezUInt16 uiDataDirID = m_MountedDataDirs.GetCount();

  ezNetworkMessage msg('FSRV', 'MNT');
  msg.GetWriter() << szDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  //dd.m_sPathOnClient = szDataDirectory;
  //dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted = true;

  return uiDataDirID;
}


void ezFileserveClient::UnmountDataDirectory(ezUInt16 uiDataDir)
{
  if (!m_Network->IsConnectedToServer())
    return;

  ezNetworkMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void ezFileserveClient::DeleteFile(ezUInt16 uiDataDir, const char* szFile)
{
  if (!m_Network->IsConnectedToServer())
    return;

  ezNetworkMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << szFile;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);
}

void ezFileserveClient::HandleFileTransferMsg(ezNetworkMessage &msg)
{
  {
    ezUInt16 uiFileID = 0;
    msg.GetReader() >> uiFileID;

    if (uiFileID != m_uiCurFileRequestID)
    {
      ezLog::Error("Fileserver is answering the wrong file request");
      return;
    }
  }

  ezUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  ezUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  // make sure we don't need to reallocate
  m_Download.Reserve(uiFileSize);

  const ezUInt32 uiStartPos = m_Download.GetCount();
  m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
  msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
}


void ezFileserveClient::HandleFileTransferFinishedMsg(ezNetworkMessage &msg)
{
  EZ_SCOPE_EXIT(m_bDownloading = false);

  {
    ezUInt16 uiFileID = 0;
    msg.GetReader() >> uiFileID;

    if (uiFileID != m_uiCurFileRequestID)
    {
      ezLog::Error("Fileserver is answering the wrong file request");
      return;
    }
  }

  ezFileserveFileState fileState;
  {
    ezInt8 iFileStatus = 0;
    msg.GetReader() >> iFileStatus;
    fileState = (ezFileserveFileState)iFileStatus;
  }

  ezInt64 iFileTimeStamp = 0;
  msg.GetReader() >> iFileTimeStamp;

  ezUInt64 uiFileHash = 0;
  msg.GetReader() >> uiFileHash;

  // nothing changed
  if (fileState == ezFileserveFileState::SameTimestamp || fileState == ezFileserveFileState::NonExistantEither)
    return;

  bool bWriteDownloadToDisk = false;
  bool bUpdateMetaFile = false;

  const ezString& sMountPoint = m_MountedDataDirs[m_uiCurFileRequestDataDir].m_sMountPoint;
  ezStringBuilder sCachedFile, sCachedMetaFile;
  BuildPathInCache(m_sCurFileRequest, sMountPoint, sCachedFile, sCachedMetaFile);

  if (fileState == ezFileserveFileState::NonExistant)
  {
    // remove them from the cache as well, if they still exist there
    ezOSFile::DeleteFile(sCachedFile);
    ezOSFile::DeleteFile(sCachedMetaFile);
    return;
  }

  // timestamp changed, but hash is still the same -> update timestamp
  if (fileState == ezFileserveFileState::SameHash)
  {
    bUpdateMetaFile = true;
  }

  if (fileState == ezFileserveFileState::Different)
  {
    bWriteDownloadToDisk = true;
    bUpdateMetaFile = true;
  }


  if (bWriteDownloadToDisk)
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

  if (bUpdateMetaFile)
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

    auto& ref = m_CachedFileStatus[m_sCurFileRequestCacheName];
    ref.m_LastCheck = ezTime::Now();
    ref.m_FileHash = uiFileHash;
    ref.m_TimeStamp = iFileTimeStamp;
    ref.m_uiDataDir = m_uiCurFileRequestDataDir;
  }
}

ezResult ezFileserveClient::DownloadFile(ezUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir)
{
  EZ_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  EZ_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  EZ_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_Network->IsConnectedToServer())
    return EZ_FAILURE;

  m_Download.Clear();

  if (bForceThisDataDir)
    m_sCurFileRequestCacheName.Set(":", m_MountedDataDirs[uiDataDirID].m_sMountPoint, "/", szFile);
  else
    m_sCurFileRequestCacheName = szFile;

  bool bAlreadyCached = false;
  auto itCache = m_CachedFileStatus.FindOrAdd(m_sCurFileRequestCacheName, &bAlreadyCached);

  if (bAlreadyCached)
  {
    const ezUInt16 uiCachedDataDir = itCache.Value().m_uiDataDir;

    if (uiCachedDataDir != 0xffff) // 0xffff means status is indeterminate
    {
      if (ezTime::Now() - itCache.Value().m_LastCheck < ezTime::Seconds(5.0f)) // only use cached state, if it wasn't too long ago
      {
        if (uiCachedDataDir >= 0xff00) // file simply does not exist
          return EZ_FAILURE;

        if (uiCachedDataDir == uiDataDirID) // file exists in THIS data directory, yay!
          return EZ_SUCCESS;

        return EZ_FAILURE; // file exists, but in another data directory (kinda yay)
      }
    }
  }

  DetermineCacheStatus(uiDataDirID, szFile, itCache.Value());

  m_Download.Clear();

  m_bDownloading = true;
  ++m_uiCurFileRequestID;
  m_sCurFileRequest = szFile;
  m_uiCurFileRequestDataDir = uiDataDirID;

  ezNetworkMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiDataDirID;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_uiCurFileRequestID;
  msg.GetWriter() << itCache.Value().m_TimeStamp;
  msg.GetWriter() << itCache.Value().m_FileHash;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_Network->UpdateNetwork();
    m_Network->ExecuteAllMessageHandlers();
  }

  if (itCache.Value().m_uiDataDir == uiDataDirID)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

void ezFileserveClient::DetermineCacheStatus(ezUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const
{
  ezStringBuilder sAbsPathFile, sAbsPathMeta;
  const auto& dd = m_MountedDataDirs[uiDataDirID];

  EZ_ASSERT_DEV(dd.m_bMounted, "Data directory {0} is not mounted", uiDataDirID);

  BuildPathInCache(szFile, dd.m_sMountPoint, sAbsPathFile, sAbsPathMeta);

  if (ezOSFile::ExistsFile(sAbsPathFile))
  {
    ezOSFile meta;
    if (meta.Open(sAbsPathMeta, ezFileMode::Read).Failed())
    {
      // cleanup, when the meta file does not exist, the data file is useless
      ezOSFile::DeleteFile(sAbsPathFile);
      return;
    }

    meta.Read(&out_Status.m_TimeStamp, sizeof(ezInt64));
    meta.Read(&out_Status.m_FileHash, sizeof(ezUInt64));

    out_Status.m_uiDataDir = uiDataDirID;
  }
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}
