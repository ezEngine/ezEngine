#include <FileservePlugin/FileservePluginPCH.h>

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
  if (m_pNetwork)
  {
    ezLog::Dev("Shutting down fileserve client");

    m_pNetwork->ShutdownConnection();
    m_pNetwork = nullptr;
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

  if (m_pNetwork == nullptr)
  {
    m_pNetwork = ezRemoteInterfaceEnet::Make(); /// \todo Somehow abstract this away ?

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

  if (!m_pNetwork->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_pNetwork->ConnectToServer('EZFS', m_sServerConnectionAddress).Failed())
      return EZ_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = ezTime::MakeFromSeconds(ezCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }

    if (m_pNetwork->WaitForConnectionToServer(timeout).Failed())
    {
      m_pNetwork->ShutdownConnection();
      ezLog::Error("Connection to ezFileserver timed out");
      return EZ_FAILURE;
    }
    else
    {
      ezLog::Success("Connected to ezFileserver '{0}", m_sServerConnectionAddress);
      m_pNetwork->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));

      m_pNetwork->Send('FSRV', 'HELO'); // be friendly
    }

    m_bFailedToConnect = false;
  }

  return EZ_SUCCESS;
}

void ezFileserveClient::UpdateClient()
{
  EZ_LOCK(m_Mutex);
  if (m_pNetwork == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
    return;

  if (!m_pNetwork->IsConnectedToServer())
  {
    if (EnsureConnected().Failed())
    {
      ezLog::Error("Fileserve connection was lost and could not be re-established.");
      ShutdownConnection();
    }
    return;
  }

  m_CurrentTime = ezTime::Now();

  m_pNetwork->ExecuteAllMessageHandlers();
}

void ezFileserveClient::AddServerAddressToTry(ezStringView sAddress)
{
  EZ_LOCK(m_Mutex);
  if (sAddress.IsEmpty())
    return;

  if (m_TryServerAddresses.Contains(sAddress))
    return;

  m_TryServerAddresses.PushBack(sAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = sAddress;
}

void ezFileserveClient::UploadFile(ezUInt16 uiDataDirID, const char* szFile, const ezDynamicArray<ezUInt8>& fileContent)
{
  EZ_LOCK(m_Mutex);

  if (m_pNetwork == nullptr)
    return;

  // update meta state and cache
  {
    const ezString& sMountPoint = m_MountedDataDirs[uiDataDirID].m_sMountPoint;
    ezStringBuilder sCachedMetaFile;
    BuildPathInCache(szFile, sMountPoint, nullptr, &sCachedMetaFile);

    ezUInt64 uiHash = 1;

    if (!fileContent.IsEmpty())
    {
      uiHash = ezHashingUtils::xxHash64(fileContent.GetData(), fileContent.GetCount(), uiHash);
    }

    WriteMetaFile(sCachedMetaFile, 0, uiHash);

    InvalidateFileCache(uiDataDirID, szFile, uiHash);
  }

  const ezUInt32 uiFileSize = fileContent.GetCount();

  ezUuid uploadGuid = ezUuid::MakeUuid();

  {
    ezRemoteMessage msg;
    msg.SetMessageID('FSRV', 'UPLH');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiFileSize;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;
    m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);
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
    msg.GetWriter().WriteBytes(&fileContent[uiNextByte], uiChunkSize).IgnoreResult();

    msg.SetMessageID('FSRV', 'UPLD');
    m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);

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

    m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished)
  {
    UpdateClient();
  }
}


void ezFileserveClient::InvalidateFileCache(ezUInt16 uiDataDirID, ezStringView sFile, ezUInt64 uiHash)
{
  EZ_LOCK(m_Mutex);
  auto& cache = m_MountedDataDirs[uiDataDirID].m_CacheStatus[sFile];
  cache.m_FileHash = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck = ezTime::MakeZero(); // will trigger a server request and that in turn will update the file timestamp

  // redirect the next access to this cache entry
  // together with the zero LastCheck that will make sure the best match gets updated as well
  m_FileDataDir[sFile] = uiDataDirID;
}

void ezFileserveClient::FillFileStatusCache(const char* szFile)
{
  EZ_LOCK(m_Mutex);
  auto it = m_FileDataDir.FindOrAdd(szFile);
  it.Value() = 0xffff; // does not exist

  for (ezUInt16 i = static_cast<ezUInt16>(m_MountedDataDirs.GetCount()); i > 0; --i)
  {
    const ezUInt16 dd = i - 1;

    if (!m_MountedDataDirs[dd].m_bMounted)
      continue;

    auto& cache = m_MountedDataDirs[dd].m_CacheStatus[szFile];

    DetermineCacheStatus(dd, szFile, cache);
    cache.m_LastCheck = ezTime::MakeZero();

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

void ezFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder* out_pAbsPath, ezStringBuilder* out_pFullPathMeta) const
{
  EZ_ASSERT_DEV(!ezPathUtils::IsAbsolutePath(szFile), "Invalid path");
  EZ_LOCK(m_Mutex);
  if (out_pAbsPath)
  {
    *out_pAbsPath = m_sFileserveCacheFolder;
    out_pAbsPath->AppendPath(szMountPoint, szFile);
    out_pAbsPath->MakeCleanPath();
  }
  if (out_pFullPathMeta)
  {
    *out_pFullPathMeta = m_sFileserveCacheMetaFolder;
    out_pFullPathMeta->AppendPath(szMountPoint, szFile);
    out_pFullPathMeta->MakeCleanPath();
  }
}

void ezFileserveClient::ComputeDataDirMountPoint(ezStringView sDataDir, ezStringBuilder& out_sMountPoint)
{
  EZ_ASSERT_DEV(sDataDir.IsEmpty() || sDataDir.EndsWith("/"), "Invalid path");

  const ezUInt32 uiMountPoint = ezHashingUtils::xxHash32String(sDataDir);
  out_sMountPoint.SetFormat("{0}", ezArgU(uiMountPoint, 8, true, 16));
}

void ezFileserveClient::GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const
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

  if (msg.GetMessageID() == 'INVC')
  {
    // invalidate caches, so that next read will go to the server

    for (auto& dd : m_MountedDataDirs)
    {
      for (auto& it : dd.m_CacheStatus)
      {
        it.Value().m_LastCheck = ezTime::MakeZero();
      }
    }

    return;
  }

  ezLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

ezUInt16 ezFileserveClient::MountDataDirectory(ezStringView sDataDirectory, ezStringView sRootName)
{
  EZ_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return 0xffff;

  ezStringBuilder sRoot = sRootName;
  sRoot.Trim(":/");

  ezStringBuilder sMountPoint;
  ComputeDataDirMountPoint(sDataDirectory, sMountPoint);

  const ezUInt16 uiDataDirID = static_cast<ezUInt16>(m_MountedDataDirs.GetCount());

  ezRemoteMessage msg('FSRV', ' MNT');
  msg.GetWriter() << sDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  // dd.m_sPathOnClient = sDataDirectory;
  // dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted = true;

  return uiDataDirID;
}


void ezFileserveClient::UnmountDataDirectory(ezUInt16 uiDataDir)
{
  EZ_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  ezRemoteMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void ezFileserveClient::DeleteFile(ezUInt16 uiDataDir, ezStringView sFile)
{
  EZ_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  ezRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);
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

  if (uiFoundInDataDir == 0xffff)         // file does not exist on server in any data dir
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
  BuildPathInCache(m_sCurFileRequest, sMountPoint, &sCachedFile, &sCachedMetaFile);

  if (fileState == ezFileserveFileState::NonExistant)
  {
    // remove them from the cache as well, if they still exist there
    ezOSFile::DeleteFile(sCachedFile).IgnoreResult();
    ezOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
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
  if (file.Open(sCachedMetaFile, ezFileOpenMode::Write).Succeeded())
  {
    file.Write(&iFileTimeStamp, sizeof(ezInt64)).IgnoreResult();
    file.Write(&uiFileHash, sizeof(ezUInt64)).IgnoreResult();

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
  if (file.Open(sCachedFile, ezFileOpenMode::Write).Succeeded())
  {
    if (!m_Download.IsEmpty())
      file.Write(m_Download.GetData(), m_Download.GetCount()).IgnoreResult();

    file.Close();
  }
  else
  {
    ezLog::Error("Failed to write download to '{0}'", sCachedFile);
  }
}

ezResult ezFileserveClient::DownloadFile(ezUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, ezStringBuilder* out_pFullPath)
{
  // bForceThisDataDir = true;
  EZ_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    ezLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  EZ_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  EZ_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_pNetwork->IsConnectedToServer())
    return EZ_FAILURE;

  bool bCachedYet = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const ezUInt16 uiUseDataDirCache = bForceThisDataDir ? uiDataDirID : itFileDataDir.Value();
  const FileCacheStatus& CacheStatus = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (m_CurrentTime - CacheStatus.m_LastCheck < ezTime::MakeFromSeconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0) // file does not exist
      return EZ_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiUseDataDirCache].m_sMountPoint, out_pFullPath, nullptr);

    return EZ_SUCCESS;
  }

  m_Download.Clear();
  m_sCurFileRequest = szFile;
  m_CurFileRequestGuid = ezUuid::MakeUuid();
  m_bDownloading = true;

  ezRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_pNetwork->Send(ezRemoteTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_pNetwork->UpdateRemoteInterface();
    m_pNetwork->ExecuteAllMessageHandlers();
  }

  if (bForceThisDataDir)
  {
    if (m_MountedDataDirs[uiDataDirID].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
      return EZ_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiDataDirID].m_sMountPoint, out_pFullPath, nullptr);

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

      if (out_pFullPath)
        BuildPathInCache(szFile, m_MountedDataDirs[uiBestDir].m_sMountPoint, out_pFullPath, nullptr);

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

  BuildPathInCache(szFile, dd.m_sMountPoint, &sAbsPathFile, &sAbsPathMeta);

  if (ezOSFile::ExistsFile(sAbsPathFile))
  {
    ezOSFile meta;
    if (meta.Open(sAbsPathMeta, ezFileOpenMode::Read).Failed())
    {
      // cleanup, when the meta file does not exist, the data file is useless
      ezOSFile::DeleteFile(sAbsPathFile).IgnoreResult();
      return;
    }

    meta.Read(&out_Status.m_TimeStamp, sizeof(ezInt64));
    meta.Read(&out_Status.m_FileHash, sizeof(ezUInt64));
  }
}

ezResult ezFileserveClient::TryReadFileserveConfig(const char* szFile, ezStringBuilder& out_Result)
{
  ezOSFile file;
  if (file.Open(szFile, ezFileOpenMode::Read).Succeeded())
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

ezResult ezFileserveClient::SearchForServerAddress(ezTime timeout /*= ezTime::MakeFromSeconds(5)*/)
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

  ezUniquePtr<ezRemoteInterfaceEnet> network = ezRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
  if (network->ConnectToServer('EZFS', szAddress, false).Failed())
    return EZ_FAILURE;

  bool bServerFound = false;
  network->SetMessageHandler('FSRV', [&bServerFound](ezRemoteMessage& ref_msg)
    {
    switch (ref_msg.GetMessageID())
    {
      case ' YES':
        bServerFound = true;
        break;
    } });

  if (network->WaitForConnectionToServer(timeout).Succeeded())
  {
    // wait for a proper response
    ezTime tStart = ezTime::Now();
    while (ezTime::Now() - tStart < timeout && !bServerFound)
    {
      network->Send('FSRV', 'RUTR');

      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }
  }

  network->ShutdownConnection();

  if (!bServerFound)
    return EZ_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  SaveCurrentConnectionInfoToDisk().IgnoreResult();
  return EZ_SUCCESS;
}

ezResult ezFileserveClient::WaitForServerInfo(ezTime timeout /*= ezTime::MakeFromSeconds(60.0 * 5)*/)
{
  EZ_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return EZ_FAILURE;

  ezUInt16 uiPort = 1042;
  ezHybridArray<ezStringBuilder, 4> sServerIPs;

  {
    ezUniquePtr<ezRemoteInterfaceEnet> network = ezRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
    network->SetMessageHandler('FSRV', [&sServerIPs, &uiPort](ezRemoteMessage& ref_msg)

      {
        switch (ref_msg.GetMessageID())
        {
          case 'MYIP':
            ref_msg.GetReader() >> uiPort;

            ezUInt8 uiCount = 0;
            ref_msg.GetReader() >> uiCount;

            sServerIPs.SetCount(uiCount);
            for (ezUInt32 i = 0; i < uiCount; ++i)
            {
              ref_msg.GetReader() >> sServerIPs[i];
            }

            break;
        } });

    EZ_SUCCEED_OR_RETURN(network->StartServer('EZIP', "2042", false));

    ezTime tStart = ezTime::Now();
    while (ezTime::Now() - tStart < timeout && sServerIPs.IsEmpty())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }

    network->ShutdownConnection();
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
      sAddress.SetFormat("{0}:{1}", ip, uiPort);

      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(500));

      if (TryConnectWithFileserver(sAddress, ezTime::MakeFromSeconds(3)).Succeeded())
        return EZ_SUCCESS;
    }

    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1000));
  }

  return EZ_FAILURE;
}

ezResult ezFileserveClient::SaveCurrentConnectionInfoToDisk() const
{
  EZ_LOCK(m_Mutex);
  ezStringBuilder sFile = ezOSFile::GetUserDataFolder("ezFileserve.txt");
  ezOSFile file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile, ezFileOpenMode::Write));

  EZ_SUCCEED_OR_RETURN(file.Write(m_sServerConnectionAddress.GetData(), m_sServerConnectionAddress.GetElementCount()));
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
