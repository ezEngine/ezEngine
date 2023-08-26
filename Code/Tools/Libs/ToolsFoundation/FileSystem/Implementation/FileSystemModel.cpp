#include <ToolsFoundation/ToolsFoundationDLL.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER) && EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

#  include <ToolsFoundation/FileSystem/FileSystemModel.h>
#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/Algorithm/HashStream.h>
#  include <Foundation/Configuration/SubSystem.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Time/Stopwatch.h>
#  include <Foundation/Utilities/Progress.h>

EZ_IMPLEMENT_SINGLETON(ezFileSystemModel);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, FileSystemModel)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezFileSystemModel);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezFileSystemModel* pDummy = ezFileSystemModel::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  thread_local ezHybridArray<ezFileChangedEvent, 2, ezStaticAllocatorWrapper> g_PostponedFiles;
  thread_local bool g_bInFileBroadcast = false;
  thread_local ezHybridArray<ezFolderChangedEvent, 2, ezStaticAllocatorWrapper> g_PostponedFolders;
  thread_local bool g_bInFolderBroadcast = false;
} // namespace

ezFolderChangedEvent::ezFolderChangedEvent(ezStringView sFile, Type type)
  : m_sPath(sFile)
  , m_Type(type)
{
}

ezFileChangedEvent::ezFileChangedEvent(ezStringView sFile, ezFileStatus status, Type type)
  : m_sPath(sFile)
  , m_Status(status)
  , m_Type(type)
{
}

bool ezFileSystemModel::IsSameFile(const ezStringView sAbsolutePathA, const ezStringView sAbsolutePathB)
{
#  if (EZ_ENABLED(EZ_SUPPORTS_CASE_INSENSITIVE_PATHS))
  return sAbsolutePathA.IsEqual_NoCase(sAbsolutePathB);
#  else
  return sAbsolutePathA.IsEqual(sAbsolutePathB);
#  endif
}

////////////////////////////////////////////////////////////////////////
// ezAssetFiles
////////////////////////////////////////////////////////////////////////

ezFileSystemModel::ezFileSystemModel()
  : m_SingletonRegistrar(this)
{
}

ezFileSystemModel::~ezFileSystemModel() = default;

void ezFileSystemModel::Initialize(const ezApplicationFileSystemConfig& fileSystemConfig, ezMap<ezString, ezFileStatus>&& referencedFiles, ezMap<ezString, ezFileStatus::Status>&& referencedFolders)
{
  {
    EZ_LOCK(m_FilesMutex);
    m_FileSystemConfig = fileSystemConfig;

    m_ReferencedFiles = std::move(referencedFiles);
    m_ReferencedFolders = std::move(referencedFolders);

    ezStringBuilder sDataDirPath;
    m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
    for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (ezFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
      {
        ezLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        m_DataDirRoots.PushBack({});
      }
      else
      {
        sDataDirPath.MakeCleanPath();
        sDataDirPath.TrimWordEnd("/");

        m_DataDirRoots.PushBack(sDataDirPath);
        // The root should always be in the model so that every file's parent folder is present in the model.
        m_ReferencedFolders.FindOrAdd(sDataDirPath).Value() = ezFileStatus::Status::Valid;
      }
    }

    m_pWatcher = EZ_DEFAULT_NEW(ezFileSystemWatcher, m_FileSystemConfig);
    m_WatcherSubscription = m_pWatcher->m_Events.AddEventHandler(ezMakeDelegate(&ezFileSystemModel::OnAssetWatcherEvent, this));
    m_pWatcher->Initialize();
    m_bInitialized = true;
  }
  FireFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset);
}


void ezFileSystemModel::Deinitialize(ezMap<ezString, ezFileStatus>* out_pReferencedFiles, ezMap<ezString, ezFileStatus::Status>* out_pReferencedFolders)
{
  {
    EZ_LOCK(m_FilesMutex);

    m_pWatcher->m_Events.RemoveEventHandler(m_WatcherSubscription);
    m_pWatcher->Deinitialize();
    m_pWatcher.Clear();

    if (out_pReferencedFiles)
    {
      m_ReferencedFiles.Swap(*out_pReferencedFiles);
    }
    if (out_pReferencedFolders)
    {
      m_ReferencedFolders.Swap(*out_pReferencedFolders);
    }
    m_ReferencedFiles.Clear();
    m_ReferencedFolders.Clear();
    m_LockedFiles.Clear();
    m_FileSystemConfig = ezApplicationFileSystemConfig();
    m_DataDirRoots.Clear();
    m_bInitialized = false;
  }
  FireFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset);
}

void ezFileSystemModel::MainThreadTick()
{
  if (m_pWatcher)
    m_pWatcher->MainThreadTick();
}

const ezFileSystemModel::LockedFiles ezFileSystemModel::GetFiles() const
{
  return LockedFiles(m_FilesMutex, &m_ReferencedFiles);
}


const ezFileSystemModel::LockedFolders ezFileSystemModel::GetFolders() const
{
  return LockedFolders(m_FilesMutex, &m_ReferencedFolders);
}

void ezFileSystemModel::NotifyOfChange(ezStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return;

  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for directory iteration.");

  ezStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath) == -1)
    return;

  HandleSingleFile(sPath, true);
}

void ezFileSystemModel::CheckFileSystem()
{
  if (!m_bInitialized)
    return;

  EZ_PROFILE_SCOPE("CheckFileSystem");

  ezUniquePtr<ezProgressRange> range = nullptr;
  if (ezThreadUtils::IsMainThread())
    range = EZ_DEFAULT_NEW(ezProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  {
    SetAllStatusUnknown();

    // check every data directory
    for (auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      ezStringBuilder sTemp;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).IgnoreResult();

      if (ezThreadUtils::IsMainThread())
        range->BeginNextStep(dd.m_sDataDirSpecialPath);

      CheckFolder(sTemp);
    }

    RemoveStaleFileInfos();
  }

  if (ezThreadUtils::IsMainThread())
  {
    range = nullptr;
  }

  FireFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset);
}


ezResult ezFileSystemModel::FindFile(ezStringView sPath, ezFileStatus& out_stat) const
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  EZ_LOCK(m_FilesMutex);
  ezMap<ezString, ezFileStatus>::ConstIterator it;
  if (ezPathUtils::IsAbsolutePath(sPath))
  {
    it = m_ReferencedFiles.Find(sPath);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      ezStringBuilder sDataDir;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(sPath);
      it = m_ReferencedFiles.Find(sDataDir);
      if (it.IsValid())
        break;
    }

    if (!it.IsValid())
    {
      // Data dir relative?
      for (const auto& dd : m_FileSystemConfig.m_DataDirs)
      {
        ezStringBuilder sDataDir;
        ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
        sDataDir.AppendPath(sPath);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  if (it.IsValid())
  {
    out_stat = it.Value();
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}


ezResult ezFileSystemModel::FindFile(ezDelegate<bool(const ezString&, const ezFileStatus&)> visitor) const
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  EZ_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    if (visitor(it.Key(), it.Value()))
      return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}


ezResult ezFileSystemModel::LinkDocument(ezStringView sAbsolutePath, const ezUuid& documentId)
{
  if (!m_bInitialized || !documentId.IsValid())
    return EZ_FAILURE;

  ezFileStatus fileStatus;
  {
    EZ_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      // Store status before updates so we can fire the unlink if a guid was already set.
      fileStatus = it.Value();
      it.Value().m_DocumentID = documentId;
    }
    else
    {
      return EZ_FAILURE;
    }
  }

  if (fileStatus.m_DocumentID != documentId)
  {
    if (fileStatus.m_DocumentID.IsValid())
    {
      FireFileChangedEvent(sAbsolutePath, fileStatus, ezFileChangedEvent::Type::DocumentUnlinked);
    }
    fileStatus.m_DocumentID = documentId;
    FireFileChangedEvent(sAbsolutePath, fileStatus, ezFileChangedEvent::Type::DocumentLinked);
  }
  return EZ_SUCCESS;
}

ezResult ezFileSystemModel::UnlinkDocument(ezStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  ezFileStatus fileStatus;
  bool bDocumentLinkChanged = false;
  {
    EZ_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged = it.Value().m_DocumentID != ezUuid();
      fileStatus = it.Value();
      it.Value().m_DocumentID = ezUuid::MakeInvalid();
    }
    else
    {
      return EZ_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(sAbsolutePath, fileStatus, ezFileChangedEvent::Type::DocumentUnlinked);
  }
  return EZ_SUCCESS;
}

ezResult ezFileSystemModel::HashFile(ezStringView sAbsolutePath, ezFileStatus& out_stat)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for hashing.");

  ezStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();

  ezFileStats statDep;
  if (ezOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
  {
    ezLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
    return EZ_FAILURE;
  }

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath2) != -1)
  {
    {
      EZ_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // We can only hash files that are tracked.
    if (out_stat.m_Status == ezFileStatus::Status::Unknown)
    {
      out_stat = HandleSingleFile(sAbsolutePath2, statDep, false);
      if (out_stat.m_Status == ezFileStatus::Status::Unknown)
      {
        ezLog::Error("Failed to hash file '{0}', update failed", sAbsolutePath2);
        return EZ_FAILURE;
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, ezTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      ezFileReader file;
      if (file.Open(sAbsolutePath2).Failed())
      {
        MarkFileLocked(sAbsolutePath2);
        ezLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return EZ_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (ezOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        ezLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return EZ_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = ezFileSystemModel::HashFile(file, nullptr);
      out_stat.m_Status = ezFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      EZ_LOCK(m_FilesMutex);
      m_ReferencedFiles.Insert(sAbsolutePath2, out_stat);
    }
    return EZ_SUCCESS;
  }
  else
  {
    {
      EZ_LOCK(m_FilesMutex);
      auto it = m_TransiendFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, ezTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      ezFileReader file;
      if (file.Open(sAbsolutePath2).Failed())
      {
        ezLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return EZ_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (ezOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        ezLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return EZ_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = ezFileSystemModel::HashFile(file, nullptr);
      out_stat.m_Status = ezFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      EZ_LOCK(m_FilesMutex);
      m_TransiendFiles.Insert(sAbsolutePath2, out_stat);
    }
    return EZ_SUCCESS;
  }
}


ezUInt64 ezFileSystemModel::HashFile(ezStreamReader& ref_inputStream, ezStreamWriter* pPassThroughStream)
{
  ezHashStreamWriter64 hsw;

  FILESYSTEM_PROFILE("HashFile");
  ezUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const ezUInt64 uiRead = ref_inputStream.ReadBytes(cachedBytes, EZ_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).AssertSuccess();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).AssertSuccess();
  }

  return hsw.GetHashValue();
}

ezResult ezFileSystemModel::ReadDocument(ezStringView sAbsolutePath, const ezDelegate<void(const ezFileStatus&, ezStreamReader&)>& callback)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  // try to read the asset file
  ezFileReader file;
  if (file.Open(sAbsolutePath) == EZ_FAILURE)
  {
    MarkFileLocked(sAbsolutePath);
    ezLog::Error("Failed to open file '{0}'", sAbsolutePath);
    return EZ_FAILURE;
  }

  // Get model state.
  ezFileStatus stat;
  {
    EZ_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (!it.IsValid())
      return EZ_FAILURE;

    stat = it.Value();
  }

  // Get current state.
  ezFileStats statDep;
  if (ezOSFile::GetFileStats(sAbsolutePath, statDep).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '{0}'", sAbsolutePath);
    return EZ_FAILURE;
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(sAbsolutePath);

  ezMemoryStreamWriter MemWriter(&storage);
  stat.m_LastModified = statDep.m_LastModificationTime;
  stat.m_Status = ezFileStatus::Status::Valid;
  stat.m_uiHash = ezFileSystemModel::HashFile(file, &MemWriter);

  if (callback.IsValid())
  {
    callback(stat, MemReader);
  }

  bool bFileChanged = false;
  {
    // Update state. No need to compare timestamps we hold a lock on the file via the reader.
    EZ_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bFileChanged = !it.Value().m_LastModified.Compare(stat.m_LastModified, ezTimestamp::CompareMode::Identical);
      it.Value() = stat;
    }
    else
    {
      EZ_REPORT_FAILURE("A file was removed from the model while we had a lock on it.");
    }

    if (bFileChanged)
    {
      FireFileChangedEvent(sAbsolutePath, stat, ezFileChangedEvent::Type::FileChanged);
    }
  }

  return EZ_SUCCESS;
}

void ezFileSystemModel::SetAllStatusUnknown()
{
  EZ_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = ezFileStatus::Status::Unknown;
  }

  for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
  {
    it.Value() = ezFileStatus::Status::Unknown;
  }
}


void ezFileSystemModel::RemoveStaleFileInfos()
{
  ezSet<ezString> unknownFiles;
  ezSet<ezString> unknownFolders;
  {
    EZ_LOCK(m_FilesMutex);
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
    {
      // search for files that existed previously but have not been found anymore recently
      if (it.Value().m_Status == ezFileStatus::Status::Unknown)
      {
        unknownFiles.Insert(it.Key());
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
    {
      // search for folders that existed previously but have not been found anymore recently
      if (it.Value() == ezFileStatus::Status::Unknown)
      {
        unknownFolders.Insert(it.Key());
      }
    }
  }

  for (const ezString& sFile : unknownFiles)
  {
    HandleSingleFile(sFile, false);
  }
  for (const ezString& sFolders : unknownFolders)
  {
    HandleSingleFile(sFolders, false);
  }
}


void ezFileSystemModel::CheckFolder(ezStringView sAbsolutePath)
{
  ezStringBuilder sAbsolutePath2 = sAbsolutePath;
  sAbsolutePath2.MakeCleanPath();
  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(sAbsolutePath2), "Only absolute paths are supported for directory iteration.");
  sAbsolutePath2.TrimWordEnd("/");

  if (sAbsolutePath2.IsEmpty())
    return;

  // We ignore any changes outside the model's data dirs.
  if (FindDataDir(sAbsolutePath2) == -1)
    return;

  bool bExists = false;
  {
    EZ_LOCK(m_FilesMutex);
    bExists = m_ReferencedFolders.Contains(sAbsolutePath2);
  }
  if (!bExists)
  {
    // If the folder does not exist yet we call NotifyOfChange which handles add / removal recursively as well.
    NotifyOfChange(sAbsolutePath2);
    return;
  }

  ezFileSystemIterator iterator;
  iterator.StartSearch(sAbsolutePath2, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

  if (!iterator.IsValid())
    return;

  ezStringBuilder sPath;

  ezSet<ezString> visitedFiles;
  ezSet<ezString> visitedFolders;
  visitedFolders.Insert(sAbsolutePath2);

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();
    if (iterator.GetStats().m_bIsDirectory)
      visitedFolders.Insert(sPath);
    else
      visitedFiles.Insert(sPath);
    HandleSingleFile(sPath, iterator.GetStats(), false);
  }

  ezDynamicArray<ezString> missingFiles;
  ezDynamicArray<ezString> missingFolders;

  {
    EZ_LOCK(m_FilesMutex);

    for (auto it = m_ReferencedFiles.LowerBound(sAbsolutePath2); it.IsValid() && it.Key().StartsWith(sAbsolutePath2); ++it)
    {
      if (!visitedFiles.Contains(it.Key()))
        missingFiles.PushBack(it.Key());
    }

    for (auto it = m_ReferencedFolders.LowerBound(sAbsolutePath2); it.IsValid() && it.Key().StartsWith(sAbsolutePath2); ++it)
    {
      if (!visitedFolders.Contains(it.Key()))
        missingFolders.PushBack(it.Key());
    }
  }

  for (const ezString& sFile : missingFiles)
  {
    HandleSingleFile(sFile, false);
  }

  // Delete sub-folders before parent folders.
  missingFolders.Sort([](const ezString& lhs, const ezString& rhs) -> bool
    { return ezStringUtils::Compare(lhs, rhs) > 0; });
  for (const ezString& sFolder : missingFolders)
  {
    HandleSingleFile(sFolder, false);
  }
}

void ezFileSystemModel::OnAssetWatcherEvent(const ezFileSystemWatcherEvent& e)
{
  switch (e.m_Type)
  {
    case ezFileSystemWatcherEvent::Type::FileAdded:
    case ezFileSystemWatcherEvent::Type::FileRemoved:
    case ezFileSystemWatcherEvent::Type::FileChanged:
    case ezFileSystemWatcherEvent::Type::DirectoryAdded:
    case ezFileSystemWatcherEvent::Type::DirectoryRemoved:
      NotifyOfChange(e.m_sPath);
      break;
  }
}

ezFileStatus ezFileSystemModel::HandleSingleFile(const ezString& sAbsolutePath, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile");

  ezFileStats Stats;
  const ezResult statCheck = ezOSFile::GetFileStats(sAbsolutePath, Stats);

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (statCheck.Succeeded() && Stats.m_sName != ezPathUtils::GetFileNameAndExtension(sAbsolutePath))
  {
    // Casing has changed.
    ezStringBuilder sCorrectCasingPath = sAbsolutePath;
    sCorrectCasingPath.ChangeFileNameAndExtension(Stats.m_sName);
    // Add new casing
    ezFileStatus res = HandleSingleFile(sCorrectCasingPath, Stats, bRecurseIntoFolders);
    // Remove old casing
    RemoveFileOrFolder(sAbsolutePath, bRecurseIntoFolders);
    return res;
  }
#  endif

  if (statCheck.Failed())
  {
    RemoveFileOrFolder(sAbsolutePath, bRecurseIntoFolders);
    return {};
  }

  return HandleSingleFile(sAbsolutePath, Stats, bRecurseIntoFolders);
}


ezFileStatus ezFileSystemModel::HandleSingleFile(const ezString& sAbsolutePath, const ezFileStats& FileStat, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile2");

  if (FileStat.m_bIsDirectory)
  {
    ezFileStatus status;
    status.m_Status = ezFileStatus::Status::Valid;

    bool bExisted = false;
    {
      EZ_LOCK(m_FilesMutex);
      auto it = m_ReferencedFolders.FindOrAdd(sAbsolutePath, &bExisted);
      it.Value() = ezFileStatus::Status::Valid;
    }

    if (!bExisted)
    {
      FireFolderChangedEvent(sAbsolutePath, ezFolderChangedEvent::Type::FolderAdded);
      if (bRecurseIntoFolders)
        CheckFolder(sAbsolutePath);
    }

    return status;
  }
  else
  {
    ezFileStatus status;
    bool bExisted = false;
    bool bFileChanged = false;
    {
      EZ_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.FindOrAdd(sAbsolutePath, &bExisted);
      ezFileStatus& value = it.Value();
      bFileChanged = !value.m_LastModified.Compare(FileStat.m_LastModificationTime, ezTimestamp::CompareMode::Identical);
      if (bFileChanged)
      {
        value.m_uiHash = 0;
      }

      // If the state is unknown, we loaded it from the cache and need to fire FileChanged to update dependent systems.
      // #TODO_ASSET This behaviors should be changed once the asset cache is stored less lossy.
      bFileChanged |= value.m_Status == ezFileStatus::Status::Unknown;
      // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
      value.m_Status = ezFileStatus::Status::Valid;
      value.m_LastModified = FileStat.m_LastModificationTime;
      status = value;
    }

    if (!bExisted)
    {
      FireFileChangedEvent(sAbsolutePath, status, ezFileChangedEvent::Type::FileAdded);
    }
    else if (bFileChanged)
    {
      FireFileChangedEvent(sAbsolutePath, status, ezFileChangedEvent::Type::FileChanged);
    }
    return status;
  }
}

void ezFileSystemModel::RemoveFileOrFolder(const ezString& sAbsolutePath, bool bRecurseIntoFolders)
{
  ezFileStatus fileStatus;
  bool bFileExisted = false;
  bool bFolderExisted = false;
  {
    EZ_LOCK(m_FilesMutex);
    if (auto it = m_ReferencedFiles.Find(sAbsolutePath); it.IsValid())
    {
      bFileExisted = true;
      fileStatus = it.Value();
      m_ReferencedFiles.Remove(it);
    }
    if (auto it = m_ReferencedFolders.Find(sAbsolutePath); it.IsValid())
    {
      bFolderExisted = true;
      m_ReferencedFolders.Remove(it);
    }
  }

  if (bFileExisted)
  {
    FireFileChangedEvent(sAbsolutePath, fileStatus, ezFileChangedEvent::Type::FileRemoved);
  }

  if (bFolderExisted)
  {
    if (bRecurseIntoFolders)
    {
      ezSet<ezString> previouslyKnownFiles;
      {
        FILESYSTEM_PROFILE("FindReferencedFiles");
        EZ_LOCK(m_FilesMutex);
        auto itlowerBound = m_ReferencedFiles.LowerBound(sAbsolutePath);
        while (itlowerBound.IsValid() && itlowerBound.Key().StartsWith(sAbsolutePath))
        {
          previouslyKnownFiles.Insert(itlowerBound.Key());
          ++itlowerBound;
        }
      }
      {
        FILESYSTEM_PROFILE("HandleRemovedFiles");
        for (const ezString& sFile : previouslyKnownFiles)
        {
          RemoveFileOrFolder(sFile, false);
        }
      }
    }
    FireFolderChangedEvent(sAbsolutePath, ezFolderChangedEvent::Type::FolderRemoved);
  }
}

void ezFileSystemModel::MarkFileLocked(ezStringView sAbsolutePath)
{
  EZ_LOCK(m_FilesMutex);
  auto it = m_ReferencedFiles.Find(sAbsolutePath);
  if (it.IsValid())
  {
    it.Value().m_Status = ezFileStatus::Status::FileLocked;
    m_LockedFiles.Insert(sAbsolutePath);
  }
}

void ezFileSystemModel::FireFileChangedEvent(ezStringView sFile, ezFileStatus fileStatus, ezFileChangedEvent::Type type)
{
  // We queue up all requests on a thread and only return once the list is empty. The reason for this is that:
  // A: We don't want to allow recursive event calling as it creates limbo states in the model and hard to debug bugs.
  // B: If a user calls NotifyOfChange, the function should only return if the event and any indirect events that were triggered by the event handlers have been processed.

  ezFileChangedEvent& e = g_PostponedFiles.ExpandAndGetRef();
  e.m_sPath = sFile;
  e.m_Status = fileStatus;
  e.m_Type = type;

  if (g_bInFileBroadcast)
  {
    return;
  }

  g_bInFileBroadcast = true;
  EZ_SCOPE_EXIT(g_bInFileBroadcast = false);

  for (ezUInt32 i = 0; i < g_PostponedFiles.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    ezFileChangedEvent tempEvent = std::move(g_PostponedFiles[i]);
    m_FileChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFiles.Clear();
}

void ezFileSystemModel::FireFolderChangedEvent(ezStringView sFile, ezFolderChangedEvent::Type type)
{
  // See comment in FireFileChangedEvent.
  ezFolderChangedEvent& e = g_PostponedFolders.ExpandAndGetRef();
  e.m_sPath = sFile;
  e.m_Type = type;

  if (g_bInFolderBroadcast)
  {
    return;
  }

  g_bInFolderBroadcast = true;
  EZ_SCOPE_EXIT(g_bInFolderBroadcast = false);

  for (ezUInt32 i = 0; i < g_PostponedFolders.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    ezFolderChangedEvent tempEvent = std::move(g_PostponedFolders[i]);
    m_FolderChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFolders.Clear();
}

ezInt32 ezFileSystemModel::FindDataDir(const ezStringView path)
{
  for (ezUInt32 i = 0; i < m_DataDirRoots.GetCount(); ++i)
  {
    if (path.StartsWith(m_DataDirRoots[i]))
    {
      return (ezInt32)i;
    }
  }
  return -1;
}

#endif
