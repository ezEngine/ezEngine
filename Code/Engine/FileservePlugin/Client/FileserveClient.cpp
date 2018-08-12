#include <PCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_IMPLEMENT_SINGLETON(ezFileserveClient);

bool ezFileserveClient::s_bEnableFileserve = true;

ezFileserveClient::ezFileserveClient()
    : m_SingletonRegistrar(this)
{
  AddServerAddressToTry("localhost:1042");

  ezStringBuilder sAddress, sSearch;

  // the app directory
  {
    sSearch = ezOSFile::GetApplicationDirectory();
    sSearch.AppendPath("ezFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  // command line argument
  AddServerAddressToTry(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // last successful IP is stored in the user directory
  {
    sSearch = ezOSFile::GetUserDataFolder("ezFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_off"))
    s_bEnableFileserve = false;

  m_CurrentTime = ezTime::Now();
}

ezFileserveClient::~ezFileserveClient()
{
  ShutdownConnection();
}

void ezFileserveClient::ShutdownConnection()
{
  if (m_Network)
  {
    ezLog::Dev("Shutting down fileserve client");

    m_Network->ShutdownConnection();
    m_Network = nullptr;
  }
}

void ezFileserveClient::ClearState()
{
  m_bDownloading = false;
  m_bWaitingForUploadFinished = false;
  m_CurFileRequestGuid = ezUuid();
  m_sCurFileRequest.Clear();
  m_Download.Clear();
}

ezResult ezFileserveClient::EnsureConnected(ezTime timeout)
{
  EZ_LOCK(m_Mutex);
  if (!s_bEnableFileserve || m_bFailedToConnect)
    return EZ_FAILURE;

  if (m_Network == nullptr)
  {
    m_Network = EZ_DEFAULT_NEW(ezRemoteInterfaceEnet); /// \todo Somehow abstract this away ?

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
  }

  if (!m_Network->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_Network->ConnectToServer('EZFS', m_sServerConnectionAddress).Failed())
      return EZ_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = ezTime::Seconds(ezCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }

    if (m_Network->WaitForConnectionToServer(timeout).Failed())
    {
      m_Network->ShutdownConnection();
      ezLog::Error("Connection to ezFileserver timed out");
      return EZ_FAILURE;
    }
    else
    {
      ezLog::Success("Connected to ezFileserver '{0}", m_sServerConnectionAddress);
      m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));

      m_Network->Send('FSRV', 'HELO'); // be friendly
    }

    m_bFailedToConnect = false;
  }

  return EZ_SUCCESS;
}

void ezFileserveClient::UpdateClient()
{
  EZ_LOCK(m_Mutex);
  if (m_Network == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
    return;

  if (!m_Network->IsConnectedToServer())
  {
    if (EnsureConnected().Failed())
    {
      ezLog::Error("Fileserve connection was lost and could not be re-established.");
      ShutdownConnection();
    }
    return;
  }

  m_CurrentTime = ezTime::Now();

  m_Network->ExecuteAllMessageHandlers();
}

void ezFileserveClient::AddServerAddressToTry(const char* szAddress)
{
  EZ_LOCK(m_Mutex);
  if (ezStringUtils::IsNullOrEmpty(szAddress))
    return;

  if (m_TryServerAddresses.Contains(szAddress))
    return;

  m_TryServerAddresses.PushBack(szAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = szAddress;
}

void ezFileserveClient::UploadFile(ezUInt16 uiDataDirID, const char* szFile, const ezDynamicArray<ezUInt8>& fileContent)
{
  EZ_LOCK(m_Mutex);

  if (m_Network == nullptr)
    return;

  // update meta state and cache
  {
    const ezString& sMountPoint = m_MountedDataDirs[uiDataDirID].m_sMountPoint;
    ezStringBuilder sCachedFile, sCachedMetaFile;
    BuildPathInCache(szFile, sMountPoint, sCachedFile, sCachedMetaFile);

    ezUInt64 uiHash = 1;

    if (!fileContent.IsEmpty())
    {
      uiHash = ezHashing::xxHash64(fileContent.GetData(), fileContent.GetCount(), uiHash);
    }

    WriteMetaFile(sCachedMetaFile, 0, uiHash);

    InvalidateFileCache(uiDataDirID, szFile, uiHash);
  }

  const ezUInt32 uiFileSize = fileContent.GetCount();

  ezUuid uploadGuid;
  uploadGuid.CreateNewUuid();

  {
    ezRemoteMessage msg;
    msg.SetMessageID('FSRV', 'UPLH');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiFileSize;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;
    m_Network->Send(ezRemoteTransmitMode::Reliable, msg);
  }

  ezUInt32 uiNextByte = 0;

  // send the file over in multiple packages of 1KB each
  // send at least one package, even for empty files

  while (uiNextByte < fileContent.GetCount())
  {
    const ezUInt16 uiChunkSize = (ezUInt16)ezMath::Min<ezUInt32>(1024, fileContent.GetCount() - uiNextByte);

    ezRemoteMessage msg;
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiChunkSize;
    msg.GetWriter().WriteBytes(&fileContent[uiNextByte], uiChunkSize);

    msg.SetMessageID('FSRV', 'UPLD');
    m_Network->Send(ezRemoteTransmitMode::Reliable, msg);

    uiNextByte += uiChunkSize;
  }

  // continuously update the network until we know the server has received the big chunk of data
  m_bWaitingForUploadFinished = true;

  // final message to server
  {
    ezRemoteMessage msg('FSRV', 'UPLF');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;

    m_Network->Send(ezRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished)
  {
    UpdateClient();
  }
}


void ezFileserveClient::InvalidateFileCache(ezUInt16 uiDataDirID, const char* szFile, ezUInt64 uiHash)
{
  EZ_LOCK(m_Mutex);
  auto& cache = m_MountedDataDirs[uiDataDirID].m_CacheStatus[szFile];
  cache.m_FileHash = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck.SetZero(); // will trigger a server request and that in turn will update the file timestamp

  // redirect the next access to this cache entry
  // together with the zero LastCheck that will make sure the best match gets updated as well
  m_FileDataDir[szFile] = uiDataDirID;
}

void ezFileserveClient::FillFileStatusCache(const char* szFile)
{
  EZ_LOCK(m_Mutex);
  auto it = m_FileDataDir.FindOrAdd(szFile);
  it.Value() = 0xffff; // does not exist

  for (ezUInt32 i = m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    const ezUInt16 dd = i - 1;

    if (!m_MountedDataDirs[dd].m_bMounted)
      continue;

    auto& cache = m_MountedDataDirs[dd].m_CacheStatus[szFile];

    DetermineCacheStatus(dd, szFile, cache);
    cache.m_LastCheck.SetZero();

    if (cache.m_TimeStamp != 0 && cache.m_FileHash != 0) // file exists
    {
      // best possible candidate
      if (it.Value() == 0xffff)
        it.Value() = dd;
    }
  }

  if (it.Value() == 0xffff)
    it.Value() = 0; // fallback
}

void ezFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath,
                                         ezStringBuilder& out_sFullPathMeta) const
{
  EZ_ASSERT_DEV(!ezPathUtils::IsAbsolutePath(szFile), "Invalid path");
  EZ_LOCK(m_Mutex);
  out_sAbsPath = m_sFileserveCacheFolder;
  out_sAbsPath.AppendPath(szMountPoint, szFile);
  out_sAbsPath.MakeCleanPath();

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(szMountPoint, szFile);
  out_sFullPathMeta.MakeCleanPath();
}

void ezFileserveClient::ComputeDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint)
{
  EZ_ASSERT_DEV(ezStringUtils::IsNullOrEmpty(szDataDir) || ezStringUtils::EndsWith(szDataDir, "/"), "Invalid path");

  const ezUInt32 uiMountPoint = ezHashing::xxHash32(szDataDir, ezStringUtils::GetStringElementCount(szDataDir));
  out_sMountPoint.Format("{0}", ezArgU(uiMountPoint, 8, true, 16));
}

void ezFileserveClient::GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath,
                                                ezStringBuilder& out_sFullPathMeta) const
{
  EZ_LOCK(m_Mutex);
  ezStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(sMountPoint);
}

void ezFileserveClient::NetworkMsgHandler(ezRemoteMessage& msg)
{
  EZ_LOCK(m_Mutex);
  if (msg.GetMessageID() == 'DWNL')
  {
    HandleFileTransferMsg(msg);
    return;
  }

  if (msg.GetMessageID() == 'DWNF')
  {
    HandleFileTransferFinishedMsg(msg);
    return;
  }

  static bool s_bReloadResources = false;

  if (msg.GetMessageID() == 'RLDR')
  {
    s_bReloadResources = true;
  }

  if (!m_bDownloading && s_bReloadResources)
  {
    EZ_BROADCAST_EVENT(ezResourceManager_ReloadAllResources);
    s_bReloadResources = false;
    return;
  }

  if (msg.GetMessageID() == 'RLDR')
    return;

  if (msg.GetMessageID() == 'UACK')
  {
    m_bWaitingForUploadFinished = false;
    return;
  }

  ezLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

ezUInt16 ezFileserveClient::MountDataDirectory(const char* szDataDirectory, const char* szRootName)
{
  EZ_LOCK(m_Mutex);
  if (!m_Network->IsConnectedToServer())
    return 0xffff;

  ezStringBuilder sRoot = szRootName;
  sRoot.Trim(":/");

  ezStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDirectory, sMountPoint);

  const ezUInt16 uiDataDirID = m_MountedDataDirs.GetCount();

  ezRemoteMessage msg('FSRV', ' MNT');
  msg.GetWriter() << szDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_Network->Send(ezRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  // dd.m_sPathOnClient = szDataDirectory;
  // dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted = true;

  return uiDataDirID;
}


void ezFileserveClient::UnmountDataDirectory(ezUInt16 uiDataDir)
{
  EZ_LOCK(m_Mutex);
  if (!m_Network->IsConnectedToServer())
    return;

  ezRemoteMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_Network->Send(ezRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void ezFileserveClient::DeleteFile(ezUInt16 uiDataDir, const char* szFile)
{
  EZ_LOCK(m_Mutex);
  if (!m_Network->IsConnectedToServer())
    return;

  InvalidateFileCache(uiDataDir, szFile, 0);

  ezRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << szFile;

  m_Network->Send(ezRemoteTransmitMode::Reliable, msg);
}

void ezFileserveClient::HandleFileTransferMsg(ezRemoteMessage& msg)
{
  EZ_LOCK(m_Mutex);
  {
    ezUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // ezLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  ezUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  ezUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  // make sure we don't need to reallocate
  m_Download.Reserve(uiFileSize);

  if (uiChunkSize > 0)
  {
    const ezUInt32 uiStartPos = m_Download.GetCount();
    m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
    msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
  }
}


void ezFileserveClient::HandleFileTransferFinishedMsg(ezRemoteMessage& msg)
{
  EZ_LOCK(m_Mutex);
  EZ_SCOPE_EXIT(m_bDownloading = false);

  {
    ezUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // ezLog::Debug("Fileserver is answering someone else");
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

  ezUInt16 uiFoundInDataDir = 0;
  msg.GetReader() >> uiFoundInDataDir;

  if (uiFoundInDataDir == 0xffff) // file does not exist on server in any data dir
  {
    m_FileDataDir[m_sCurFileRequest] = 0; // placeholder

    for (ezUInt32 i = 0; i < m_MountedDataDirs.GetCount(); ++i)
    {
      auto& ref = m_MountedDataDirs[i].m_CacheStatus[m_sCurFileRequest];
      ref.m_FileHash = 0;
      ref.m_TimeStamp = 0;
      ref.m_LastCheck = m_CurrentTime;
    }

    return;
  }
  else
  {
    m_FileDataDir[m_sCurFileRequest] = uiFoundInDataDir;

    auto& ref = m_MountedDataDirs[uiFoundInDataDir].m_CacheStatus[m_sCurFileRequest];
    ref.m_FileHash = uiFileHash;
    ref.m_TimeStamp = iFileTimeStamp;
    ref.m_LastCheck = m_CurrentTime;
  }

  // nothing changed
  if (fileState == ezFileserveFileState::SameTimestamp || fileState == ezFileserveFileState::NonExistantEither)
    return;

  const ezString& sMountPoint = m_MountedDataDirs[uiFoundInDataDir].m_sMountPoint;
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
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }

  if (fileState == ezFileserveFileState::Different)
  {
    WriteDownloadToDisk(sCachedFile);
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }
}


void ezFileserveClient::WriteMetaFile(ezStringBuilder sCachedMetaFile, ezInt64 iFileTimeStamp, ezUInt64 uiFileHash)
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

void ezFileserveClient::WriteDownloadToDisk(ezStringBuilder sCachedFile)
{
  EZ_LOCK(m_Mutex);
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

ezResult ezFileserveClient::DownloadFile(ezUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir)
{
  EZ_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    ezLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  EZ_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  EZ_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_Network->IsConnectedToServer())
    return EZ_FAILURE;

  bool bCachedYet = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const ezUInt16 uiUseDataDirCache = bForceThisDataDir ? uiDataDirID : itFileDataDir.Value();
  const FileCacheStatus& CacheStatus = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (m_CurrentTime - CacheStatus.m_LastCheck < ezTime::Seconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0) // file does not exist
      return EZ_FAILURE;

    return EZ_SUCCESS;
  }

  m_Download.Clear();
  m_sCurFileRequest = szFile;
  m_CurFileRequestGuid.CreateNewUuid();
  m_bDownloading = true;

  ezRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_Network->Send(ezRemoteTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_Network->UpdateRemoteInterface();
    m_Network->ExecuteAllMessageHandlers();
  }

  if (bForceThisDataDir)
  {
    if (m_MountedDataDirs[uiDataDirID].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
      return EZ_FAILURE;

    return EZ_SUCCESS;
  }
  else
  {
    const ezUInt16 uiBestDir = itFileDataDir.Value();
    if (uiBestDir == uiDataDirID) // best match is still this? -> success
    {
      // file does not exist
      if (m_MountedDataDirs[uiBestDir].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
        return EZ_FAILURE;

      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }
}

void ezFileserveClient::DetermineCacheStatus(ezUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const
{
  EZ_LOCK(m_Mutex);
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
  }
}

ezResult ezFileserveClient::TryReadFileserveConfig(const char* szFile, ezStringBuilder& out_Result)
{
  ezOSFile file;
  if (file.Open(szFile, ezFileMode::Read).Succeeded())
  {
    ezUInt8 data[64]; // an IP + port should not be longer than 22 characters

    ezStringBuilder res;

    data[file.Read(data, 63)] = 0;
    res = (const char*)data;
    res.Trim(" \t\n\r");

    if (res.IsEmpty())
      return EZ_FAILURE;

    // has to contain a port number
    if (res.FindSubString(":") == nullptr)
      return EZ_FAILURE;

    // otherwise could be an arbitrary string
    out_Result = res;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezFileserveClient::SearchForServerAddress(ezTime timeout /*= ezTime::Seconds(5)*/)
{
  EZ_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return EZ_FAILURE;

  ezStringBuilder sAddress;

  // add the command line argument again, in case this was modified since the constructor ran
  // will not change anything, if this is a duplicate
  AddServerAddressToTry(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, ""));

  // go through the available options
  for (ezInt32 idx = m_TryServerAddresses.GetCount() - 1; idx >= 0; --idx)
  {
    if (TryConnectWithFileserver(m_TryServerAddresses[idx], timeout).Succeeded())
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezFileserveClient::TryConnectWithFileserver(const char* szAddress, ezTime timeout) const
{
  EZ_LOCK(m_Mutex);
  if (ezStringUtils::IsNullOrEmpty(szAddress))
    return EZ_FAILURE;

  ezLog::Info("File server address: '{0}' ({1} sec)", szAddress, timeout.GetSeconds());

  ezRemoteInterfaceEnet network; /// \todo Abstract this somehow ?
  if (network.ConnectToServer('EZFS', szAddress, false).Failed())
    return EZ_FAILURE;

  bool bServerFound = false;
  network.SetMessageHandler('FSRV', [&bServerFound](ezRemoteMessage& msg) {
    switch (msg.GetMessageID())
    {
      case ' YES':
        bServerFound = true;
        break;
    }
  });

  if (network.WaitForConnectionToServer(timeout).Succeeded())
  {
    // wait for a proper response
    ezTime tStart = ezTime::Now();
    while (ezTime::Now() - tStart < timeout && !bServerFound)
    {
      network.Send('FSRV', 'RUTR');

      ezThreadUtils::Sleep(ezTime::Milliseconds(100));

      network.UpdateRemoteInterface();
      network.ExecuteAllMessageHandlers();
    }
  }

  network.ShutdownConnection();

  if (!bServerFound)
    return EZ_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  SaveCurrentConnectionInfoToDisk();
  return EZ_SUCCESS;
}

ezResult ezFileserveClient::WaitForServerInfo(ezTime timeout /*= ezTime::Seconds(60.0 * 5)*/)
{
  EZ_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return EZ_FAILURE;

  ezUInt16 uiPort = 1042;
  ezHybridArray<ezStringBuilder, 4> sServerIPs;

  {
    ezRemoteInterfaceEnet network; /// \todo Abstract this somehow ?
    network.SetMessageHandler('FSRV', [&sServerIPs, &uiPort](ezRemoteMessage& msg)

                              {
                                switch (msg.GetMessageID())
                                {
                                  case 'MYIP':
                                    msg.GetReader() >> uiPort;

                                    ezUInt8 uiCount = 0;
                                    msg.GetReader() >> uiCount;

                                    sServerIPs.SetCount(uiCount);
                                    for (ezUInt32 i = 0; i < uiCount; ++i)
                                    {
                                      msg.GetReader() >> sServerIPs[i];
                                    }

                                    break;
                                }
                              });

    network.StartServer('EZIP', "2042", false);

    ezTime tStart = ezTime::Now();
    while (ezTime::Now() - tStart < timeout && sServerIPs.IsEmpty())
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(1));

      network.UpdateRemoteInterface();
      network.ExecuteAllMessageHandlers();
    }

    network.ShutdownConnection();
  }

  if (sServerIPs.IsEmpty())
    return EZ_FAILURE;

  // network connections are unreliable and surprisingly slow sometimes
  // we just got an IP from a server, so we know it's there and we should be able to connect to it
  // still this often fails the first few times
  // so we try this several times and waste some time in between and hope that at some point the connection succeeds
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    ezStringBuilder sAddress;
    for (auto& ip : sServerIPs)
    {
      sAddress.Format("{0}:{1}", ip, uiPort);

      ezThreadUtils::Sleep(ezTime::Milliseconds(500));

      if (TryConnectWithFileserver(sAddress, ezTime::Seconds(3)).Succeeded())
        return EZ_SUCCESS;
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(1000));
  }

  return EZ_FAILURE;
}

ezResult ezFileserveClient::SaveCurrentConnectionInfoToDisk() const
{
  EZ_LOCK(m_Mutex);
  ezStringBuilder sFile = ezOSFile::GetUserDataFolder("ezFileserve.txt");
  ezOSFile file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile, ezFileMode::Write));

  file.Write(m_sServerConnectionAddress.GetData(), m_sServerConnectionAddress.GetElementCount());
  file.Close();

  return EZ_SUCCESS;
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}



EZ_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveClient);
