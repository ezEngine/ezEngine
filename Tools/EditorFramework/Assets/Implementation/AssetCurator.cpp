#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <CoreUtils/Other/Progress.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

EZ_IMPLEMENT_SINGLETON(ezAssetCurator);

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Setup
////////////////////////////////////////////////////////////////////////

ezAssetCurator::ezAssetCurator()
  : m_SingletonRegistrar(this)
{
  m_bRunUpdateTask = false;
  m_bRunProcessTask = false;
  m_bNeedToReloadResources = false;
  m_pUpdateTask = nullptr;
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
}

ezAssetCurator::~ezAssetCurator()
{
  Deinitialize();

  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
}

void ezAssetCurator::Initialize(const ezApplicationFileSystemConfig& cfg)
{
  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;
  m_sActivePlatform = "PC";

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp = ezApplicationConfig::GetSdkRootDirectory();
    sTemp.AppendPath(dd.m_sSdkRootRelativePath);
    sTemp.MakeCleanPath();

    ezDirectoryWatcher* pWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);
    ezResult res = pWatcher->OpenDirectory(sTemp, ezDirectoryWatcher::Watch::Reads | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Subdirectories);
    if (res.Failed())
      ezLog::Error("Failed to init directory watcher for dir '%s'", sTemp.GetData());
    m_Watchers.PushBack(pWatcher);
  }

  CheckFileSystem();
}

void ezAssetCurator::Deinitialize()
{
  ShutdownUpdateTask();
  ShutdownProcessTask();

  {
    ezLock<ezMutex> ml(m_CuratorMutex);

    for (ezDirectoryWatcher* pWatcher : m_Watchers)
    {
      EZ_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
    m_ReferencedFiles.Clear();

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      EZ_DEFAULT_DELETE(it.Value());
    }
    m_KnownAssets.Clear();
    m_TransformStateUnknown.Clear();
    m_TransformStateNeedsTransform.Clear();
    m_TransformStateNeedsThumbnail.Clear();
  }

  // Broadcast reset.
  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
}


void ezAssetCurator::SetActivePlatform(const char* szPlatform)
{
  {
    EZ_LOCK(m_CuratorMutex);
    m_sActivePlatform = szPlatform;
  }

  ezAssetCuratorEvent e;
  e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;

  m_Events.Broadcast(e);
}

void WatcherCallback(const char* szDirectory, const char* szFilename, ezDirectoryWatcher::Action action)
{
  switch (action)
  {
  case ezDirectoryWatcher::Action::Added:
  case ezDirectoryWatcher::Action::Removed:
  case ezDirectoryWatcher::Action::Modified:
  case ezDirectoryWatcher::Action::RenamedOldName:
  case ezDirectoryWatcher::Action::RenamedNewName:
    {
      ezStringBuilder sTemp = szDirectory;
      sTemp.AppendPath(szFilename);
      ezAssetCurator::GetSingleton()->NotifyOfFileChange(sTemp);
    }
  }
}

void ezAssetCurator::MainThreadTick()
{
  EZ_LOCK(m_CuratorMutex);

  static bool bReentry = false;
  if (bReentry)
    return;

  bReentry = true;

  for (ezDirectoryWatcher* pWatcher : m_Watchers)
  {
    pWatcher->EnumerateChanges([pWatcher](const char* szFilename, ezDirectoryWatcher::Action action)
    {
      WatcherCallback(pWatcher->GetDirectory(), szFilename, action);
    });
  }

  /// \todo Christopher wants this faster!!

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid();)
  {
    if (it.Value()->m_ExistanceState == ezAssetInfo::ExistanceState::FileAdded)
    {
      it.Value()->m_ExistanceState = ezAssetInfo::ExistanceState::FileUnchanged;

      // Broadcast reset.
      ezAssetCuratorEvent e;
      e.m_AssetGuid = it.Key();
      e.m_pInfo = it.Value();
      e.m_Type = ezAssetCuratorEvent::Type::AssetAdded;
      m_Events.Broadcast(e);

      ++it;
    }
    else if (it.Value()->m_ExistanceState == ezAssetInfo::ExistanceState::FileRemoved)
    {
      // Broadcast reset.
      ezAssetCuratorEvent e;
      e.m_AssetGuid = it.Key();
      e.m_pInfo = it.Value();
      e.m_Type = ezAssetCuratorEvent::Type::AssetRemoved;
      m_Events.Broadcast(e);

      // Remove asset
      auto oldIt = it;
      ++it;

      EZ_DEFAULT_DELETE(oldIt.Value());
      m_KnownAssets.Remove(oldIt.Key());
    }
    else
      ++it;
  }

  for (const ezUuid& guid : m_TransformStateChanged)
  {
    ezAssetCuratorEvent e;
    e.m_AssetGuid = guid;
    e.m_pInfo = GetAssetInfo(guid);
    e.m_Type = ezAssetCuratorEvent::Type::AssetUpdated;

    if (e.m_pInfo != nullptr)
      m_Events.Broadcast(e);
  }
  m_TransformStateChanged.Clear();

  RunNextUpdateTask();
  RunNextProcessTask();

  if (!ezQtEditorApp::GetSingleton()->IsInHeadlessMode())
  {
    if (m_bNeedToReloadResources)
    {
      m_bNeedToReloadResources = false;
      WriteAssetTables();
    }
  }
  bReentry = false;
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator High Level Functions
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::TransformAllAssets(const char* szPlatform)
{
  ezProgressRange range("Transforming Assets", 1 + m_KnownAssets.GetCount(), true);

  EZ_LOCK(m_CuratorMutex);
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (range.WasCanceled())
      break;

    ezAssetInfo* pAssetInfo = it.Value();

    range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirRelativePath).GetData());

    auto res = ProcessAsset(pAssetInfo, szPlatform);
    if (res.m_Result.Failed())
    {
      ezLog::Error("%s (%s)", res.m_sMessage.GetData(), pAssetInfo->m_sDataDirRelativePath.GetData());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  ezAssetCurator::GetSingleton()->WriteAssetTables(szPlatform);
}

ezStatus ezAssetCurator::TransformAsset(const ezUuid& assetGuid, const char* szPlatform)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return ezStatus("Transform failed, unknown asset.");

  return ProcessAsset(pInfo, szPlatform);
}

ezStatus ezAssetCurator::CreateThumbnail(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return ezStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr);
}

ezResult ezAssetCurator::WriteAssetTables(const char* szPlatform /* = nullptr*/)
{
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");

  ezResult res = EZ_SUCCESS;

  ezStringBuilder s;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    s.Set(ezApplicationConfig::GetSdkRootDirectory(), "/", dd.m_sSdkRootRelativePath);

    if (WriteAssetTable(s, szPlatform).Failed())
      res = EZ_FAILURE;
  }

  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadAssetLUT";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

  msg.m_sWhatToDo = "ReloadResources";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

  return res;
}


void ezAssetCurator::RestartProcessTask()
{
  EZ_LOCK(m_CuratorMutex);
  m_bRunProcessTask = true;

  RunNextProcessTask();

  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetCurator::ShutdownProcessTask()
{
  ezDynamicArray<ezProcessTask*> Tasks;
  {
    EZ_LOCK(m_CuratorMutex);
    Tasks = m_ProcessTasks;
    m_ProcessTasks.Clear();
    m_bRunProcessTask = false;
  }

  if (!Tasks.IsEmpty())
  {
    for (ezProcessTask* pTask : Tasks)
    {
      ezTaskSystem::WaitForTask((ezTask*)pTask);

      // Delete and remove under lock.
      EZ_LOCK(m_CuratorMutex);
      EZ_DEFAULT_DELETE(pTask);
    }
  }

  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}


bool ezAssetCurator::IsProcessTaskRunning() const
{
  return m_bRunProcessTask;
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const ezAssetCurator::ezLockedAssetInfo ezAssetCurator::FindAssetInfo(const char* szPathOrGuid) const
{
  EZ_LOCK(m_CuratorMutex);
  ezStringBuilder sPath = szPathOrGuid;

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    return GetAssetInfo2(ezConversionUtils::ConvertStringToUuid(sPath));
  }

  sPath.MakeCleanPath();

  if (sPath.IsAbsolutePath())
  {
    ezString sPath2 = sPath;
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath2))
      return ezLockedAssetInfo();

    sPath = sPath2;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->m_sDataDirRelativePath.IsEqual_NoCase(sPath))
      return ezLockedAssetInfo(m_CuratorMutex, it.Value());
  }

  return ezLockedAssetInfo();
}

const ezAssetCurator::ezLockedAssetInfo ezAssetCurator::GetAssetInfo2(const ezUuid& assetGuid) const
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    return ezLockedAssetInfo(m_CuratorMutex, pAssetInfo);
  }
  return ezLockedAssetInfo();
}

const ezAssetCurator::ezLockedAssetTable ezAssetCurator::GetKnownAssets() const
{
  return ezLockedAssetTable(m_CuratorMutex, &m_KnownAssets);
}

ezUInt64 ezAssetCurator::GetAssetDependencyHash(ezUuid assetGuid)
{
  return GetAssetHash(assetGuid, false);
}

ezUInt64 ezAssetCurator::GetAssetReferenceHash(ezUuid assetGuid)
{
  return GetAssetHash(assetGuid, true);
}

ezAssetInfo::TransformState ezAssetCurator::IsAssetUpToDate(const ezUuid& assetGuid, const char* szPlatform, const ezDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash)
{
  out_AssetHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(assetGuid);
  out_ThumbHash = 0;
  if (out_AssetHash == 0)
  {
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::MissingDependency);
    return ezAssetInfo::TransformState::MissingDependency;
  }
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  const ezString sTargetFile = static_cast<ezAssetDocumentManager*>(pTypeDescriptor->m_pManager)->GetFinalOutputFileName(pTypeDescriptor, GetAssetInfo(assetGuid)->m_sAbsolutePath, sPlatform);
  auto flags = static_cast<ezAssetDocumentManager*>(pTypeDescriptor->m_pManager)->GetAssetDocumentTypeFlags(pTypeDescriptor);

  if (ezAssetDocumentManager::IsResourceUpToDate(out_AssetHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion(), sTargetFile))
  {
    if (flags.IsSet(ezAssetDocumentFlags::SupportsThumbnail))
    {
      out_ThumbHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(assetGuid);
      if (out_ThumbHash == 0)
      {
        UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::MissingReference);
        return ezAssetInfo::TransformState::MissingReference;
      }

      ezString sAssetFile;
      {
        EZ_LOCK(m_CuratorMutex);
        sAssetFile = GetAssetInfo(assetGuid)->m_sAbsolutePath;
      }
      if (!ezAssetDocumentManager::IsThumbnailUpToDate(out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion(), sAssetFile))
      {
        UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::NeedsThumbnail);
        return ezAssetInfo::TransformState::NeedsThumbnail;
      }
    }

    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::UpToDate);
    return ezAssetInfo::TransformState::UpToDate;
  }
  else
  {
    if (flags.IsSet(ezAssetDocumentFlags::SupportsThumbnail))
    {
      out_ThumbHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(assetGuid);
    }
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::NeedsTransform);
    return ezAssetInfo::TransformState::NeedsTransform;
  }
}

void ezAssetCurator::GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezUInt32& out_uiNumUnknown, ezUInt32& out_uiNumNeedTransform, ezUInt32& out_uiNumNeedThumb, ezUInt32& out_uiNumMissingDep, ezUInt32& out_uiNumMissingRef)
{
  EZ_LOCK(m_CuratorMutex);

  out_uiNumAssets = m_KnownAssets.GetCount();
  out_uiNumUnknown = m_TransformStateUnknown.GetCount();
  out_uiNumNeedTransform = m_TransformStateNeedsTransform.GetCount();
  out_uiNumNeedThumb = m_TransformStateNeedsThumbnail.GetCount();
  out_uiNumMissingDep = m_TransformStateMissingDependency.GetCount();
  out_uiNumMissingRef = m_TransformStateMissingReference.GetCount();
}

ezString ezAssetCurator::FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const
{
  ezStringBuilder sAssetPath(szAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir(ezApplicationConfig::GetSdkRootDirectory(), "/", dd.m_sSdkRootRelativePath);

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  EZ_REPORT_FAILURE("Could not find data directory for asset '%s", szAbsoluteAssetPath);
  return ezApplicationConfig::GetSdkRootDirectory();
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::NotifyOfFileChange(const char* szAbsolutePath)
{
  ezStringBuilder sPath(szAbsolutePath);
  sPath.MakeCleanPath();
  HandleSingleFile(sPath);
  MainThreadTick();
}

void ezAssetCurator::NotifyOfAssetChange(const ezUuid& assetGuid)
{
  UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::Unknown);
}

void ezAssetCurator::UpdateAssetLastAccessTime(const ezUuid& assetGuid)
{
  ezAssetInfo* pAssetInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return;

  pAssetInfo->m_LastAccess = ezTime::Now();
}

void ezAssetCurator::CheckFileSystem()
{
  ezTime before = ezTime::Now();

  ezProgressRange range("Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  ezLock<ezMutex> ml(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  BuildFileExtensionSet(m_ValidAssetExtensions);

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp = ezApplicationConfig::GetSdkRootDirectory();
    sTemp.AppendPath(dd.m_sSdkRootRelativePath);

    range.BeginNextStep(dd.m_sSdkRootRelativePath);

    IterateDataDirectory(sTemp, m_ValidAssetExtensions);
  }

  RemoveStaleFileInfos();

  // Broadcast reset.
  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  RestartUpdateTask();

  ezTime after = ezTime::Now();
  ezTime diff = after - before;
  ezLog::Info("Asset Curator Refresh Time: %.3f ms", diff.GetMilliseconds());
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator private functions
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::DocumentManagerEventHandler(const ezDocumentManager::Event& r)
{
  // Invoke new scan
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Processing
////////////////////////////////////////////////////////////////////////

ezStatus ezAssetCurator::ProcessAsset(ezAssetInfo* pAssetInfo, const char* szPlatform)
{
  for (const auto& dep : pAssetInfo->m_Info.m_FileDependencies)
  {
    if (ezAssetInfo* pInfo = GetAssetInfo(dep))
    {
      ezStatus res = ProcessAsset(pInfo, szPlatform);
      if (res.m_Result.Failed())
        return res;
    }
  }

  for (const auto& ref : pAssetInfo->m_Info.m_FileReferences)
  {
    if (ezAssetInfo* pInfo = GetAssetInfo(ref))
    {
      ezStatus res = ProcessAsset(pInfo, szPlatform);
      if (res.m_Result.Failed())
        return res;
    }
  }


  // Find the descriptor for the asset.
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pAssetInfo->m_sAbsolutePath, false, pTypeDesc).Failed())
  {
    return ezStatus("The asset '%s' could not be queried for its ezDocumentTypeDescriptor, skipping transform!", pAssetInfo->m_sDataDirRelativePath.GetData());
  }

  // Skip assets that cannot be auto-transformed.
  EZ_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('%s')", pAssetInfo->m_sDataDirRelativePath.GetData());
  auto assetFlags = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager)->GetAssetDocumentTypeFlags(pTypeDesc);
  if (assetFlags.IsAnySet(ezAssetDocumentFlags::DisableTransform | ezAssetDocumentFlags::OnlyTransformManually))
    return ezStatus(EZ_SUCCESS);

  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  auto state = IsAssetUpToDate(pAssetInfo->m_Info.m_DocumentID, szPlatform, pTypeDesc, uiHash, uiThumbHash);
  if (state == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS);

  if (state == ezAssetInfo::TransformState::MissingDependency)
  {
    return ezStatus("Missing dependency for asset '%s', can't transform.", pAssetInfo->m_sAbsolutePath.GetData());
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocumentImmediate(pAssetInfo->m_sAbsolutePath, false, false);

  if (pDoc == nullptr)
    return ezStatus("Could not open asset document '%s'", pAssetInfo->m_sDataDirRelativePath.GetData());

  ezStatus ret(EZ_SUCCESS);
  ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
  if (state == ezAssetInfo::TransformState::NeedsTransform || state == ezAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform))
  {
    ret = pAsset->TransformAsset(szPlatform);
  }
  
  if (state == ezAssetInfo::TransformState::MissingReference)
  {
    return ezStatus("Missing reference for asset '%s', can't create thumbnail.", pAssetInfo->m_sAbsolutePath.GetData());
  }

  if (assetFlags.IsSet(ezAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform))
  {
    if (ret.m_Result.Succeeded() && state <= ezAssetInfo::TransformState::NeedsThumbnail)
    {
      ret = pAsset->CreateThumbnail();
    }
  }

  if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

  return ret;
}


ezAssetInfo* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid)
{
  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo; 
  return nullptr;
}

ezAssetInfo* ezAssetCurator::GetAssetInfo(const ezString& sAssetGuid)
{
  if (sAssetGuid.IsEmpty())
    return nullptr;

  if (ezConversionUtils::IsStringUuid(sAssetGuid))
  {
    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sAssetGuid);

    ezAssetInfo* pInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pInfo))
      return pInfo;
  }

  return nullptr;
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath)
{
  ezFileStats Stats;
  if (ezOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      it.Value().m_Timestamp.Invalidate();
      it.Value().m_uiHash = 0;
      it.Value().m_Status = FileStatus::Status::Unknown;
    }
    return;
  }
  EZ_LOCK(m_CuratorMutex);

  // make sure the set exists, but don't update it
  // this is only updated in CheckFileSystem
  if (m_ValidAssetExtensions.IsEmpty())
    BuildFileExtensionSet(m_ValidAssetExtensions);

  HandleSingleFile(sAbsolutePath, m_ValidAssetExtensions, Stats);
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat)
{
  EZ_LOCK(m_CuratorMutex);

  ezStringBuilder sExt = ezPathUtils::GetFileExtension(sAbsolutePath);
  sExt.ToLower();

  // store information for every file, even when it is no asset, it might be a dependency for some asset
  auto& RefFile = m_ReferencedFiles[sAbsolutePath];

  // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
  RefFile.m_Status = FileStatus::Status::Valid;

  bool fileChanged = !RefFile.m_Timestamp.Compare(FileStat.m_LastModificationTime, ezTimestamp::CompareMode::Identical);
  if (fileChanged)
  {
    RefFile.m_Timestamp.Invalidate();
    RefFile.m_uiHash = 0;
    auto it = m_InverseDependency.Find(sAbsolutePath);
    if (it.IsValid())
    {
      for (const ezUuid& guid : it.Value())
      {
        UpdateAssetTransformState(guid, ezAssetInfo::TransformState::Unknown);
      }
    }

    auto it2 = m_InverseReferences.Find(sAbsolutePath);
    if (it2.IsValid())
    {
      for (const ezUuid& guid : it2.Value())
      {
        UpdateAssetTransformState(guid, ezAssetInfo::TransformState::Unknown);
      }
    }
  }

  // check that this is an asset type that we know
  if (!validExtensions.Contains(sExt))
  {
    return;
  }

  // the file is a known asset type
  // so make sure it gets a valid GUID assigned

  // File hasn't change, early out.
  if (RefFile.m_AssetGuid.IsValid() && !fileChanged)
    return;

  // store the folder of the asset
  {
    ezStringBuilder sAssetFolder = sAbsolutePath;
    sAssetFolder = sAssetFolder.GetFileDirectory();

    m_AssetFolders.Insert(sAssetFolder);
  }

  // This will update the timestamp for assets.
  EnsureAssetInfoUpdated(sAbsolutePath);
}

ezResult ezAssetCurator::WriteAssetTable(const char* szDataDirectory, const char* szPlatform)
{
  ezString sPlatform = szPlatform;
  if (sPlatform.IsEmpty())
    sPlatform = m_sActivePlatform;

  ezStringBuilder sDataDir = szDataDirectory;
  sDataDir.MakeCleanPath();

  ezStringBuilder sFinalPath(sDataDir, "/AssetCache/", sPlatform, ".ezAidlt");
  sFinalPath.MakeCleanPath();


  ezDeferredFileWriter file;
  file.SetOutput(sFinalPath); 

  ezStringBuilder sTemp;
  ezString sResourcePath;

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    sTemp = it.Value()->m_sAbsolutePath;

    // ignore all assets that are not located in this data directory
    if (!sTemp.IsPathBelowFolder(sDataDir))
      continue;

    const ezUuid& guid = it.Key();
    sResourcePath = it.Value()->m_pManager->GenerateRelativeResourceFileName(sDataDir, sTemp, sPlatform);

    sTemp.Format("%s;%s\n", ezConversionUtils::ToString(guid).GetData(), sResourcePath.GetData());

    file.WriteBytes(sTemp.GetData(), sTemp.GetElementCount());
  }

  if (file.Close().Failed())
  {
    ezLog::Error("Failed to open asset lookup table file ('%s')", sFinalPath.GetData());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Update Task
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::RestartUpdateTask()
{
  ezLock<ezMutex> ml(m_CuratorMutex);
  m_bRunUpdateTask = true;

  RunNextUpdateTask();
}

void ezAssetCurator::ShutdownUpdateTask()
{
  {
    ezLock<ezMutex> ml(m_CuratorMutex);
    m_bRunUpdateTask = false;
  }

  if (m_pUpdateTask)
  {
    ezTaskSystem::WaitForTask((ezTask*)m_pUpdateTask);

    ezLock<ezMutex> ml(m_CuratorMutex);
    EZ_DEFAULT_DELETE(m_pUpdateTask);
  }
}

bool ezAssetCurator::GetNextAssetToUpdate(ezUuid& guid, ezStringBuilder& out_sAbsPath)
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  while (!m_TransformStateUnknown.IsEmpty())
  {
    auto it = m_TransformStateUnknown.GetIterator();
    guid = it.Key();

    m_TransformStateUnknown.Remove(it);

    auto pAssetInfo = GetAssetInfo(guid);
    if (pAssetInfo == nullptr)
      continue;

    EZ_ASSERT_DEBUG(pAssetInfo->m_TransformState == ezAssetInfo::TransformState::Unknown, "State caching desynced!");
    UpdateAssetTransformState(guid, ezAssetInfo::TransformState::Updating);
    m_TransformStateUpdating.Insert(guid);
    out_sAbsPath = GetAssetInfo(guid)->m_sAbsolutePath;
    return true;
  }

  return false;
}

void ezAssetCurator::OnUpdateTaskFinished(ezTask* pTask)
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  RunNextUpdateTask();
}

void ezAssetCurator::RunNextUpdateTask()
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  if (!m_bRunUpdateTask || m_TransformStateUnknown.IsEmpty())
    return;

  if (m_pUpdateTask == nullptr)
  {
    m_pUpdateTask = EZ_DEFAULT_NEW(ezUpdateTask);
    m_pUpdateTask->SetOnTaskFinished(ezMakeDelegate(&ezAssetCurator::OnUpdateTaskFinished, this));
  }

  if (m_pUpdateTask->IsTaskFinished())
    ezTaskSystem::StartSingleTask(m_pUpdateTask, ezTaskPriority::FileAccess);
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Process Task
////////////////////////////////////////////////////////////////////////

bool ezAssetCurator::GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath)
{
  bool bComplete = true;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager)->GetAssetDocumentTypeFlags(pTypeDesc);
    if (flags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::DisableTransform))
      return false;

  }

  auto TestFunc = [this, &bComplete](ezSet<ezString>& Files) -> ezAssetInfo*
  {
    for (const auto& sFile : Files)
    {
      if (ezAssetInfo* pFileInfo = GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
        case ezAssetInfo::TransformState::Unknown:
        case ezAssetInfo::TransformState::Updating:
          {
            bComplete = false;
            break;
          }
        case ezAssetInfo::TransformState::NeedsTransform:
        case ezAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
            break;
          }
        case ezAssetInfo::TransformState::UpToDate:
          continue;
        }
      }
    }
    return nullptr;
  };
  
  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info.m_FileDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath);
  }

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info.m_FileReferences))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath);
  }

  if (bComplete)
  {
    out_guid = pInfo->m_Info.m_DocumentID;
    out_sAbsPath = pInfo->m_sAbsolutePath;
    return true;
  }

  return false;
}

bool ezAssetCurator::GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath)
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  for (auto it = m_TransformStateNeedsTransform.GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath);
      if (bRes)
        return true;
    }
  }

  for (auto it = m_TransformStateNeedsThumbnail.GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath);
      if (bRes)
        return true;
    }
  }

  return false;
}


void ezAssetCurator::OnProcessTaskFinished(ezTask* pTask)
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  RunNextProcessTask();
}


void ezAssetCurator::RunNextProcessTask()
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  if (!m_bRunProcessTask || (m_TransformStateNeedsTransform.IsEmpty() && m_TransformStateNeedsThumbnail.IsEmpty()))
    return;

  if (m_ProcessTasks.IsEmpty())
  { 
    m_ProcessTaskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::LongRunning);

    const ezUInt32 uiWorkerCount = 1;// ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks);
    for (ezUInt32 i = 0; i < uiWorkerCount; ++i)
    {
      ezProcessTask* pTask = EZ_DEFAULT_NEW(ezProcessTask);
      pTask->SetOnTaskFinished(ezMakeDelegate(&ezAssetCurator::OnProcessTaskFinished, this));
      //ezTaskSystem::AddTaskToGroup(m_ProcessTaskGroup, pTask);
      m_ProcessTasks.PushBack(pTask);
    }
  }

  for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); ++i)
  {
    if (m_ProcessTasks[i]->IsTaskFinished())
    {
      ezTaskSystem::StartSingleTask(m_ProcessTasks[i], ezTaskPriority::LongRunning);
    }
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Check File System Helper
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::SetAllAssetStatusUnknown()
{
  // tags all known files as unknown, such that we can later remove files
  // that can not be found anymore

  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = FileStatus::Status::Unknown;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), ezAssetInfo::TransformState::Unknown);
  }
}

void ezAssetCurator::RemoveStaleFileInfos()
{
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid();)
  {
    // search for files that existed previously but have not been found anymore recently
    if (it.Value().m_Status == FileStatus::Status::Unknown)
    {
      // if it was a full asset, not just any kind of reference
      if (it.Value().m_AssetGuid.IsValid())
      {
        // retrieve the ezAssetInfo data for this file
        ezAssetInfo* pCache = m_KnownAssets[it.Value().m_AssetGuid];

        // sanity check: if the ezAssetInfo really belongs to this file, remove it as well
        if (it.Key() == pCache->m_sAbsolutePath)
        {
          EZ_ASSERT_DEBUG(pCache->m_Info.m_DocumentID == it.Value().m_AssetGuid, "GUID mismatch, curator state probably corrupt!");
          m_KnownAssets.Remove(pCache->m_Info.m_DocumentID);
          EZ_DEFAULT_DELETE(pCache);
        }
      }

      // remove the file from the list
      it = m_ReferencedFiles.Remove(it);
    }
    else
      ++it;
  }
}

void ezAssetCurator::BuildFileExtensionSet(ezSet<ezString>& AllExtensions)
{
  ezStringBuilder sTemp;
  AllExtensions.Clear();

  const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();

  for (const auto& desc : allDesc)
  {
    if (desc->m_pManager->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
    {
      sTemp = desc->m_sFileExtension;
      sTemp.ToLower();

      AllExtensions.Insert(sTemp);
    }
  }
}

void ezAssetCurator::IterateDataDirectory(const char* szDataDir, const ezSet<ezString>& validExtensions)
{
  ezStringBuilder sDataDir = szDataDir;
  sDataDir.MakeCleanPath();

  while (sDataDir.EndsWith("/"))
    sDataDir.Shrink(0, 1);

  if (sDataDir.IsEmpty())
    return;

  ezFileSystemIterator iterator;
  if (iterator.StartSearch(sDataDir, true, false).Failed())
    return;

  ezStringBuilder sPath;

  do
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sFileName);
    sPath.MakeCleanPath();

    HandleSingleFile(sPath, validExtensions, iterator.GetStats());
  }
  while (iterator.Next().Succeeded());
}

