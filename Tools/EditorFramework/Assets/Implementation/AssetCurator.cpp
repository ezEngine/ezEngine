#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <QDir>
#include <atomic>

#define EZ_CURATOR_CACHE_VERSION 2
#define EZ_CURATOR_CACHE_FILE_VERSION 6

EZ_IMPLEMENT_SINGLETON(ezAssetCurator);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezAssetCurator);
  }

  ON_CORE_SHUTDOWN
  {
    ezAssetCurator* pDummy = ezAssetCurator::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFileStatus, ezNoBase, 3, ezRTTIDefaultAllocator<ezFileStatus>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Timestamp", m_Timestamp),
    EZ_MEMBER_PROPERTY("Hash", m_uiHash),
    EZ_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezFileStatus& uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(ezFileStatus));
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezFileStatus& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(ezFileStatus));
  return Stream;
}

void ezAssetInfo::Update(ezUniquePtr<ezAssetInfo>& rhs)
{
  m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pManager = rhs->m_pManager;
  m_sAbsolutePath = std::move(rhs->m_sAbsolutePath);
  m_sDataDirRelativePath = std::move(rhs->m_sDataDirRelativePath);
  m_Info = std::move(rhs->m_Info);
  m_LastAssetDependencyHash = rhs->m_LastAssetDependencyHash;
  m_MissingDependencies = rhs->m_MissingDependencies;
  m_MissingReferences = rhs->m_MissingReferences;
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

ezStringView ezSubAsset::GetName() const
{
  if (m_bMainAsset)
    return ezPathUtils::GetFileName(m_pAssetInfo->m_sDataDirRelativePath.GetData(),
                                    m_pAssetInfo->m_sDataDirRelativePath.GetData() +
                                        m_pAssetInfo->m_sDataDirRelativePath.GetElementCount());
  else
    return m_Data.m_sName;
}


void ezSubAsset::GetSubAssetIdentifier(ezStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_sDataDirRelativePath;

  if (!m_bMainAsset)
  {
    out_sPath.Append("|", m_Data.m_sName);
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Setup
////////////////////////////////////////////////////////////////////////

ezAssetCurator::ezAssetCurator()
    : m_SingletonRegistrar(this)
{
  m_bRunUpdateTask = false;
  m_bNeedToReloadResources = false;
  m_pUpdateTask = nullptr;
}

ezAssetCurator::~ezAssetCurator()
{
  EZ_ASSERT_DEBUG(m_KnownAssets.IsEmpty(), "Need to call Deinitialize before curator is deleted.");
}

void ezAssetCurator::StartInitialize(const ezApplicationFileSystemConfig& cfg)
{
  EZ_PROFILE("StartInitialize");

  {
    EZ_LOG_BLOCK("SetupAssetProfiles");

    SetupDefaultAssetProfiles();
    if (LoadAssetProfiles().Failed())
    {
      ezLog::Warning("Asset profiles file does not exist or contains invalid data. Setting up default profiles.");
      SaveAssetProfiles();
    }
  }

  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;

  {
    EZ_PROFILE("Watchers");
    for (auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      ezStringBuilder sTemp;
      if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      {
        ezLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
        continue;
      }

      ezDirectoryWatcher* pWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);
      ezResult res = pWatcher->OpenDirectory(sTemp, ezDirectoryWatcher::Watch::Reads | ezDirectoryWatcher::Watch::Writes |
                                                        ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Renames |
                                                        ezDirectoryWatcher::Watch::Subdirectories);

      if (res.Failed())
      {
        EZ_DEFAULT_DELETE(pWatcher);
        ezLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
        continue;
      }

      m_Watchers.PushBack(pWatcher);
    }
    m_WatcherTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Watcher Update", [this]() {
      CURATOR_PROFILE("Watcher");
      for (ezDirectoryWatcher* pWatcher : m_Watchers)
      {
        pWatcher->EnumerateChanges([pWatcher, this](const char* szFilename, ezDirectoryWatcherAction action) {
          ezStringBuilder sTemp = pWatcher->GetDirectory();
          sTemp.AppendPath(szFilename);
          m_WatcherResults.PushBack(sTemp);
        });
      }
    });
  }

  ezDelegateTask<void>* pInitTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Initialize Curator", [this]() {
    EZ_LOCK(m_CuratorMutex);
    m_bInitStarted = true;
    LoadCaches();
    CheckFileSystem();

    // As we fired a AssetListReset in CheckFileSystem, set everything new to FileUnchanged or
    // we would fire an added call for every asset.
    for (auto it = m_KnownSubAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_ExistanceState == ezAssetExistanceState::FileAdded)
      {
        it.Value().m_ExistanceState = ezAssetExistanceState::FileUnchanged;
      }
    }
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_ExistanceState == ezAssetExistanceState::FileAdded)
      {
        it.Value()->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
      }
    }
    SaveCaches();
  });
  pInitTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });
  m_bInitStarted = false;
  ezTaskSystem::StartSingleTask(pInitTask, ezTaskPriority::FileAccessHighPriority);

  {
    ezAssetCuratorEvent e;
    e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetCurator::WaitForInitialize()
{
  EZ_PROFILE("WaitForInitialize");
  while (m_bInitStarted == false)
  {
    ezThreadUtils::YieldTimeSlice();
  }

  EZ_LOCK(m_CuratorMutex);
  ProcessAllCoreAssets();
  // Broadcast reset.
  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
}

void ezAssetCurator::Deinitialize()
{
  EZ_PROFILE("Deinitialize");

  SaveAssetProfiles();

  ShutdownUpdateTask();
  ezAssetProcessor::GetSingleton()->ShutdownProcessTask();
  SaveCaches();

  {
    EZ_LOCK(m_CuratorMutex);

    ezTaskSystem::WaitForGroup(m_WatcherGroup);
    EZ_DEFAULT_DELETE(m_WatcherTask);
    m_WatcherResults.Clear();

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
    m_KnownSubAssets.Clear();
    m_KnownAssets.Clear();
    m_TransformStateStale.Clear();

    for (int i = 0; i < ezAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Clear();
    }
  }

  // Broadcast reset.
  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  ClearAssetProfiles();
}

void ezAssetCurator::MainThreadTick()
{
  CURATOR_PROFILE("MainThreadTick");

  static std::atomic<bool> bReentry = false;
  if (bReentry)
    return;

  bReentry = true;

  if (m_WatcherTask && ezTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    for (const ezString& sFile : m_WatcherResults)
    {
      NotifyOfFileChange(sFile);
    }
    m_WatcherResults.Clear();
    m_WatcherGroup = ezTaskSystem::StartSingleTask(m_WatcherTask, ezTaskPriority::LongRunningHighPriority);
  }

  EZ_LOCK(m_CuratorMutex);
  ezHybridArray<ezAssetInfo*, 32> deletedAssets;
  for (const ezUuid& guid : m_SubAssetChanged)
  {
    ezSubAsset* pInfo = GetSubAssetInternal(guid);
    ezAssetCuratorEvent e;
    e.m_AssetGuid = guid;
    e.m_pInfo = pInfo;
    e.m_Type = ezAssetCuratorEvent::Type::AssetUpdated;

    if (pInfo != nullptr)
    {
      if (pInfo->m_ExistanceState == ezAssetExistanceState::FileAdded)
      {
        pInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        e.m_Type = ezAssetCuratorEvent::Type::AssetAdded;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == ezAssetExistanceState::FileRemoved)
      {
        e.m_Type = ezAssetCuratorEvent::Type::AssetRemoved;
        m_Events.Broadcast(e);

        if (pInfo->m_bMainAsset)
        {
          deletedAssets.PushBack(pInfo->m_pAssetInfo);
        }
        m_KnownAssets.Remove(guid);
        m_KnownSubAssets.Remove(guid);
      }
      else // Either ezAssetInfo::ExistanceState::FileModified or tranform changed
      {
        pInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        e.m_Type = ezAssetCuratorEvent::Type::AssetUpdated;
        m_Events.Broadcast(e);
      }
    }
  }
  m_SubAssetChanged.Clear();

  // Delete file asset info after all the sub-assets have been handled (so no ref exist to it anymore).
  for (ezAssetInfo* pInfo : deletedAssets)
  {
    EZ_DEFAULT_DELETE(pInfo);
  }

  RunNextUpdateTask();
  ezAssetProcessor::GetSingleton()->RunNextProcessTask();

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

void ezAssetCurator::TransformAllAssets(const ezPlatformProfile* pAssetProfile)
{
  ezUInt32 uiNumStepsLeft = m_KnownAssets.GetCount();
  ezProgressRange range("Transforming Assets", 1 + uiNumStepsLeft, true);

  EZ_LOCK(m_CuratorMutex);
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (range.WasCanceled())
      break;

    ezAssetInfo* pAssetInfo = it.Value();

    if (uiNumStepsLeft > 0)
    {
      // it can happen that the number of known assets changes while we are processing them
      // in this case the progress bar may assert that the number of steps completed is larger than
      // what was specified before
      // since this is a valid case, we just stop updating the progress bar, in case more assets are detected

      range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirRelativePath).GetData());
      --uiNumStepsLeft;
    }

    auto res = ProcessAsset(pAssetInfo, pAssetProfile, false);
    if (res.m_Result.Failed())
    {
      ezLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirRelativePath);
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  ezAssetCurator::GetSingleton()->WriteAssetTables(pAssetProfile);
}

void ezAssetCurator::ResaveAllAssets()
{
  ezProgressRange range("Re-saving all Assets", 1 + m_KnownAssets.GetCount(), true);

  EZ_LOCK(m_CuratorMutex);

  ezDynamicArray<ezUuid> sortedAssets;
  sortedAssets.Reserve(m_KnownAssets.GetCount());

  ezMap<ezUuid, ezSet<ezUuid>> dependencies;

  ezSet<ezUuid> accu;

  for (auto itAsset = m_KnownAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    auto it2 = dependencies.Insert(itAsset.Key(), ezSet<ezUuid>());
    for (const ezString& dep : itAsset.Value()->m_Info->m_AssetTransformDependencies)
    {
      if (ezConversionUtils::IsStringUuid(dep))
      {
        it2.Value().Insert(ezConversionUtils::ConvertStringToUuid(dep));
      }
    }
  }

  while (!dependencies.IsEmpty())
  {
    bool bDeadEnd = true;
    for (auto it = dependencies.GetIterator(); it.IsValid(); ++it)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(it.Value()))
      {
        sortedAssets.PushBack(it.Key());
        accu.Insert(it.Key());
        dependencies.Remove(it);
        bDeadEnd = false;
        break;
      }
    }

    if (bDeadEnd)
    {
      // Just take the next one in and hope for the best.
      auto it = dependencies.GetIterator();
      sortedAssets.PushBack(it.Key());
      accu.Insert(it.Key());
      dependencies.Remove(it);
    }
  }

  for (ezUInt32 i = 0; i < sortedAssets.GetCount(); i++)
  {
    if (range.WasCanceled())
      break;

    ezAssetInfo* pAssetInfo = GetAssetInfo(sortedAssets[i]);
    EZ_ASSERT_DEBUG(pAssetInfo, "Should not happen as data was derived from known assets list.");
    range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirRelativePath).GetData());

    auto res = ResaveAsset(pAssetInfo);
    if (res.m_Result.Failed())
    {
      ezLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirRelativePath);
    }
  }
}

ezStatus ezAssetCurator::TransformAsset(const ezUuid& assetGuid, bool bTriggeredManually, const ezPlatformProfile* pAssetProfile)
{
  EZ_LOCK(m_CuratorMutex);

  ezStopwatch timer;
  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return ezStatus("Transform failed, unknown asset.");

  auto res = ProcessAsset(pInfo, pAssetProfile, bTriggeredManually);
  ezLog::Info("Transform asset time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
  return res;
}

ezStatus ezAssetCurator::CreateThumbnail(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return ezStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr, false);
}

ezResult ezAssetCurator::WriteAssetTables(const ezPlatformProfile* pAssetProfile /* = nullptr*/)
{
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");

  ezResult res = EZ_SUCCESS;

  ezStringBuilder s;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, s);
    s.Append("/");

    if (WriteAssetTable(s, pAssetProfile).Failed())
      res = EZ_FAILURE;
  }

  if (pAssetProfile == nullptr || pAssetProfile == GetActiveAssetProfile())
  {
    ezSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

    msg.m_sWhatToDo = "ReloadResources";
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  return res;
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const ezAssetCurator::ezLockedSubAsset ezAssetCurator::FindSubAsset(const char* szPathOrGuid) const
{
  EZ_LOCK(m_CuratorMutex);

  if (ezConversionUtils::IsStringUuid(szPathOrGuid))
  {
    return GetSubAsset(ezConversionUtils::ConvertStringToUuid(szPathOrGuid));
  }

  auto FindAsset = [this](ezStringView path) -> ezAssetInfo* {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    ezUInt32 uiMinLength = 0xFFFFFFFF;
    ezAssetInfo* pBestInfo = nullptr;

    if (path.IsEmpty())
      return nullptr;

    const ezStringBuilder sPathWithSlash("/", path.GetData());

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_sDataDirRelativePath.EndsWith_NoCase(path.GetData()))
      {
        // endswith -> could also be equal
        if (path.IsEqual_NoCase(it.Value()->m_sDataDirRelativePath.GetData()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_sDataDirRelativePath.EndsWith_NoCase(sPathWithSlash))
        {
          const ezUInt32 uiLength = it.Value()->m_sDataDirRelativePath.GetElementCount();
          if (uiLength < uiMinLength)
          {
            uiMinLength = uiLength;
            pBestInfo = it.Value();
          }
        }
      }
    }

    return pBestInfo;
  };

  const char* szSeparator = ezStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    ezStringBuilder mainAsset;
    mainAsset.SetSubString_FromTo(szPathOrGuid, szSeparator);

    ezStringView subAsset(szSeparator + 1);
    if (ezAssetInfo* pAssetInfo = FindAsset(mainAsset))
    {
      for (const ezUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto it = m_KnownSubAssets.Find(sub);
        if (it.IsValid() && subAsset.IsEqual_NoCase(it.Value().GetName()))
        {
          return ezLockedSubAsset(m_CuratorMutex, &it.Value());
        }
      }
    }
  }

  ezStringBuilder sPath = szPathOrGuid;
  sPath.MakeCleanPath();
  if (sPath.IsAbsolutePath())
  {
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
      return ezLockedSubAsset();
  }

  if (ezAssetInfo* pAssetInfo = FindAsset(sPath))
  {
    auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
    return ezLockedSubAsset(m_CuratorMutex, &itSub.Value());
  }
  return ezLockedSubAsset();
}

const ezAssetCurator::ezLockedSubAsset ezAssetCurator::GetSubAsset(const ezUuid& assetGuid) const
{
  EZ_LOCK(m_CuratorMutex);

  auto it = m_KnownSubAssets.Find(assetGuid);
  if (it.IsValid())
  {
    const ezSubAsset* pAssetInfo = &(it.Value());
    return ezLockedSubAsset(m_CuratorMutex, pAssetInfo);
  }
  return ezLockedSubAsset();
}

const ezAssetCurator::ezLockedSubAssetTable ezAssetCurator::GetKnownSubAssets() const
{
  return ezLockedSubAssetTable(m_CuratorMutex, &m_KnownSubAssets);
}

ezUInt64 ezAssetCurator::GetAssetDependencyHash(ezUuid assetGuid)
{
  return GetAssetHash(assetGuid, false);
}

ezUInt64 ezAssetCurator::GetAssetReferenceHash(ezUuid assetGuid)
{
  return GetAssetHash(assetGuid, true);
}

ezAssetInfo::TransformState ezAssetCurator::IsAssetUpToDate(const ezUuid& assetGuid, const ezPlatformProfile* pAssetProfile,
                                                            const ezDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_AssetHash,
                                                            ezUInt64& out_ThumbHash)
{
  CURATOR_PROFILE("IsAssetUpToDate");

  ezAssetDocumentManager* pManager = static_cast<ezAssetDocumentManager*>(pTypeDescriptor->m_pManager);

  const ezUInt64 uiProfileHash = pManager->GetAssetProfileHash();

  out_AssetHash = GetAssetDependencyHash(assetGuid) + uiProfileHash;
  out_ThumbHash = GetAssetReferenceHash(assetGuid);
  if (out_AssetHash == 0)
  {
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::MissingDependency);
    return ezAssetInfo::TransformState::MissingDependency;
  }
  if (out_ThumbHash == 0)
  {
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::MissingReference);
    return ezAssetInfo::TransformState::MissingReference;
  }

  auto flags = pManager->GetAssetDocumentTypeFlags(pTypeDescriptor);
  ezString sAssetFile;
  ezSet<ezString> outputs;
  {
    EZ_LOCK(m_CuratorMutex);
    ezAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    sAssetFile = pAssetInfo->m_sAbsolutePath;
    outputs = pAssetInfo->m_Info->m_Outputs;
  }

  if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
  {
    if (flags.IsSet(ezAssetDocumentFlags::SupportsThumbnail))
    {
      if (!ezAssetDocumentManager::IsThumbnailUpToDate(sAssetFile, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
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
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::NeedsTransform);
    return ezAssetInfo::TransformState::NeedsTransform;
  }
}

void ezAssetCurator::GetAssetTransformStats(ezUInt32& out_uiNumAssets,
                                            ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT>& out_count)
{
  EZ_LOCK(m_CuratorMutex);
  out_count.SetCountUninitialized(ezAssetInfo::TransformState::COUNT);
  for (int i = 0; i < ezAssetInfo::TransformState::COUNT; i++)
  {
    out_count[i] = m_TransformState[i].GetCount();
  }

  out_uiNumAssets = m_KnownAssets.GetCount();
}

ezString ezAssetCurator::FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const
{
  ezStringBuilder sAssetPath(szAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir);

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  EZ_REPORT_FAILURE("Could not find data directory for asset '{0}", szAbsoluteAssetPath);
  return ezFileSystem::GetSdkRootDirectory();
}

ezResult ezAssetCurator::FindBestMatchForFile(ezStringBuilder& sFile, ezArrayPtr<ezString> AllowedFileExtensions) const
{
  sFile.MakeCleanPath();

  ezStringBuilder testName = sFile;

  for (const auto& ext : AllowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (ezFileSystem::ExistsFile(testName))
    {
      sFile = testName;
      return EZ_SUCCESS;
    }
  }

  testName = sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    sFile = "";
    return EZ_FAILURE;
  }

  if (ezPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    ezPathUtils::MakeValidFilename(testName, '_', sFile);
    return EZ_FAILURE;
  }

  EZ_LOCK(m_CuratorMutex);

  auto SearchFile = [this](ezStringBuilder& name) -> bool {
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_Status != ezFileStatus::Status::Valid)
        continue;

      const ezString& key = it.Key();

      if (key.EndsWith_NoCase(name))
      {
        name = it.Key();
        return true;
      }
    }

    return false;
  };

  // search for the full name
  {
    testName.Prepend("/"); // make sure to not find partial names

    for (const auto& ext : AllowedFileExtensions)
    {
      testName.ChangeFileExtension(ext);

      if (SearchFile(testName))
        goto found;
    }
  }

  return EZ_FAILURE;

found:
  if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(testName))
  {
    sFile = testName;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
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
  InvalidateAssetTransformState(assetGuid);
}

void ezAssetCurator::UpdateAssetLastAccessTime(const ezUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (!it.IsValid())
    return;

  it.Value().m_LastAccess = ezTime::Now();
}

void ezAssetCurator::CheckFileSystem()
{
  EZ_PROFILE("CheckFileSystem");
  ezStopwatch sw;

  ComputeAllDocumentManagerAssetProfileHashes();

  ezProgressRange* range = nullptr;
  if (ezThreadUtils::IsMainThread())
    range = EZ_DEFAULT_NEW(ezProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  EZ_LOCK(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  BuildFileExtensionSet(m_ValidAssetExtensions);

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp);

    if (ezThreadUtils::IsMainThread())
      range->BeginNextStep(dd.m_sDataDirSpecialPath);

    IterateDataDirectory(sTemp, m_ValidAssetExtensions);
  }

  RemoveStaleFileInfos();

  if (ezThreadUtils::IsMainThread())
  {
    EZ_DEFAULT_DELETE(range);
    // Broadcast reset only if we are on the main thread.
    // Otherwise we are on the init task thread and the reset will be called on the main thread by WaitForInitialize.
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  RestartUpdateTask();

  ezLog::Debug("Asset Curator Refresh Time: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Processing
////////////////////////////////////////////////////////////////////////

ezStatus ezAssetCurator::ProcessAsset(ezAssetInfo* pAssetInfo, const ezPlatformProfile* pAssetProfile, bool bTriggeredManually)
{

  for (const auto& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
  {
    if (ezAssetInfo* pInfo = GetAssetInfo(dep))
    {
      EZ_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, false));
    }
  }

  ezStatus resReferences(EZ_SUCCESS);
  for (const auto& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
  {
    if (ezAssetInfo* pInfo = GetAssetInfo(ref))
    {
      resReferences = ProcessAsset(pInfo, pAssetProfile, false);
      if (resReferences.m_Result.Failed())
        break;
    }
  }

  // Find the descriptor for the asset.
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pAssetInfo->m_sAbsolutePath, false, pTypeDesc).Failed())
  {
    return ezStatus(ezFmt("The asset '{0}' could not be queried for its ezDocumentTypeDescriptor, skipping transform!",
                          pAssetInfo->m_sDataDirRelativePath));
  }

  EZ_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<ezAssetDocument>(),
                "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_sDataDirRelativePath);

  auto assetFlags = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager)->GetAssetDocumentTypeFlags(pTypeDesc);

  // Skip assets that cannot be auto-transformed.
  {
    if (assetFlags.IsAnySet(ezAssetDocumentFlags::DisableTransform))
      return ezStatus(EZ_SUCCESS);

    if (!bTriggeredManually && assetFlags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually))
      return ezStatus(EZ_SUCCESS);
  }

  // If references are not complete and we generate thumbnails on transform we can cancel right away.
  if (assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) && resReferences.m_Result.Failed())
  {
    return resReferences;
  }

  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezAssetInfo::TransformState state = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);

  if (bTriggeredManually)
  {
    state = ezAssetInfo::NeedsTransform;
  }

  if (state == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS);

  if (state == ezAssetInfo::TransformState::MissingDependency)
  {
    return ezStatus(ezFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_sAbsolutePath));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirRelativePath));

  ezStatus ret(EZ_SUCCESS);
  ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
  if (state == ezAssetInfo::TransformState::NeedsTransform ||
      (state == ezAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform)))
  {
    ret = pAsset->TransformAsset(bTriggeredManually, pAssetProfile);
  }

  if (state == ezAssetInfo::TransformState::MissingReference)
  {
    return ezStatus(ezFmt("Missing reference for asset '{0}', can't create thumbnail.", pAssetInfo->m_sAbsolutePath));
  }

  if (assetFlags.IsSet(ezAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) &&
      !resReferences.m_Result.Failed())
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


ezStatus ezAssetCurator::ResaveAsset(ezAssetInfo* pAssetInfo)
{
  bool bWasOpen = false;
  ezDocument* pDoc = pAssetInfo->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirRelativePath));

  ezStatus ret = pDoc->SaveDocument(true);

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

ezSubAsset* ezAssetCurator::GetSubAssetInternal(const ezUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath)
{
  CURATOR_PROFILE("HandleSingleFile");
  EZ_LOCK(m_CuratorMutex);

  ezFileStats Stats;
  if (ezOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    // this is a bit tricky:
    // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
    // instead we could set it as modified
    // but then when it was only moved or renamed that means we have another document with the same GUID
    // so once the user would save the now modified document, we would end up with two documents with the same GUID
    // so, for now, since this is probably a rare case anyway, we just close the document without asking
    ezDocumentManager::EnsureDocumentIsClosedInAllManagers(sAbsolutePath);

    if (ezFileStatus* pFileStatus = m_ReferencedFiles.GetValue(sAbsolutePath))
    {
      pFileStatus->m_Timestamp.Invalidate();
      pFileStatus->m_uiHash = 0;
      pFileStatus->m_Status = ezFileStatus::Status::Unknown;

      ezUuid guid = pFileStatus->m_AssetGuid;
      if (guid.IsValid())
      {
        ezAssetInfo* pAssetInfo = m_KnownAssets[guid];
        UntrackDependencies(pAssetInfo);
        RemoveAssetTransformState(guid);
        SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileRemoved);
        pFileStatus->m_AssetGuid = ezUuid();
      }
    }
    return;
  }

  // make sure the set exists, but don't update it
  // this is only updated in CheckFileSystem
  if (m_ValidAssetExtensions.IsEmpty())
    BuildFileExtensionSet(m_ValidAssetExtensions);

  HandleSingleFile(sAbsolutePath, m_ValidAssetExtensions, Stats);
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat)
{
  CURATOR_PROFILE("HandleSingleFile2");
  EZ_LOCK(m_CuratorMutex);

  ezStringBuilder sExt = ezPathUtils::GetFileExtension(sAbsolutePath);
  sExt.ToLower();

  // store information for every file, even when it is no asset, it might be a dependency for some asset
  auto& RefFile = m_ReferencedFiles[sAbsolutePath];

  // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
  RefFile.m_Status = ezFileStatus::Status::Valid;

  bool fileChanged = !RefFile.m_Timestamp.Compare(FileStat.m_LastModificationTime, ezTimestamp::CompareMode::Identical);
  if (fileChanged)
  {
    RefFile.m_Timestamp.Invalidate();
    RefFile.m_uiHash = 0;
    if (RefFile.m_AssetGuid.IsValid())
      InvalidateAssetTransformState(RefFile.m_AssetGuid);

    auto it = m_InverseDependency.Find(sAbsolutePath);
    if (it.IsValid())
    {
      for (const ezUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(sAbsolutePath);
    if (it2.IsValid())
    {
      for (const ezUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
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

ezResult ezAssetCurator::WriteAssetTable(const char* szDataDirectory, const ezPlatformProfile* pAssetProfile0 /*= nullptr*/)
{
  const ezPlatformProfile* pAssetProfile = pAssetProfile0;

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  ezStringBuilder sDataDir = szDataDirectory;
  sDataDir.MakeCleanPath();

  ezStringBuilder sFinalPath(sDataDir, "/AssetCache/", pAssetProfile->GetConfigName(), ".ezAidlt");
  sFinalPath.MakeCleanPath();

  ezStringBuilder sTemp, sTemp2;
  ezString sResourcePath;

  ezMap<ezString, ezString> GuidToPath;

  {
    for (auto& man : ezAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
        continue;

      ezAssetDocumentManager* pManager = static_cast<ezAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(sDataDir, pAssetProfile, GuidToPath);
    }
  }

  // TODO: Iterate over m_KnownSubAssets instead
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    sTemp = it.Value()->m_sAbsolutePath;

    // ignore all assets that are not located in this data directory
    if (!sTemp.IsPathBelowFolder(sDataDir))
      continue;

    ezAssetDocumentManager* pManager = it.Value()->m_pManager;

    auto WriteEntry = [this, &sDataDir, &pAssetProfile, &GuidToPath, pManager, &sTemp, &sTemp2](const ezUuid& guid) {
      ezSubAsset* pSub = GetSubAssetInternal(guid);
      ezString sEntry = pManager->GetAssetTableEntry(pSub, sDataDir, pAssetProfile);

      // it is valid to write no asset table entry, if no redirection is required
      // this is used by decal assets for instance
      if (!sEntry.IsEmpty())
      {
        ezConversionUtils::ToString(guid, sTemp2);

        GuidToPath[sTemp2] = sEntry;
      }
    };

    WriteEntry(it.Key());
    for (const ezUuid& subGuid : it.Value()->m_SubAssets)
    {
      WriteEntry(subGuid);
    }
  }

  ezDeferredFileWriter file;
  file.SetOutput(sFinalPath);

  for (auto it = GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    const ezString& guid = it.Key();
    const ezString& path = it.Value();

    file.WriteBytes(guid.GetData(), guid.GetElementCount());
    file.WriteBytes(";", 1);
    file.WriteBytes(path.GetData(), path.GetElementCount());
    file.WriteBytes("\n", 1);
  }

  if (file.Close().Failed())
  {
    ezLog::Error("Failed to open asset lookup table file ('{0}')", sFinalPath);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::ProcessAllCoreAssets()
{
  EZ_PROFILE("ProcessAllCoreAssets");
  if (ezQtUiServices::IsHeadless())
    return;

  // The 'Core Assets' are always transformed for the PC platform,
  // as they are needed to run the editor properly
  const ezPlatformProfile* pAssetProfile = GetDevelopmentAssetProfile();

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sCoreCollectionPath;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sCoreCollectionPath);

    ezStringBuilder sName = sCoreCollectionPath.GetFileName();
    sName.Append(".ezCollectionAsset");
    sCoreCollectionPath.AppendPath(sName);

    QFile coreCollection(sCoreCollectionPath.GetData());
    if (coreCollection.exists())
    {
      auto pSubAsset = FindSubAsset(sCoreCollectionPath);
      if (pSubAsset)
      {
        ezStatus resReferences(EZ_SUCCESS);
        for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_RuntimeDependencies)
        {
          if (ezAssetInfo* pInfo = GetAssetInfo(ref))
          {
            resReferences = ProcessAsset(pInfo, pAssetProfile, false);
            if (resReferences.m_Result.Failed())
              break;
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Update Task
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::RestartUpdateTask()
{
  EZ_LOCK(m_CuratorMutex);
  m_bRunUpdateTask = true;

  RunNextUpdateTask();
}

void ezAssetCurator::ShutdownUpdateTask()
{
  {
    EZ_LOCK(m_CuratorMutex);
    m_bRunUpdateTask = false;
  }

  if (m_pUpdateTask)
  {
    ezTaskSystem::WaitForTask((ezTask*)m_pUpdateTask);

    EZ_LOCK(m_CuratorMutex);
    EZ_DEFAULT_DELETE(m_pUpdateTask);
  }
}

bool ezAssetCurator::GetNextAssetToUpdate(ezUuid& guid, ezStringBuilder& out_sAbsPath)
{
  EZ_LOCK(m_CuratorMutex);

  while (!m_TransformStateStale.IsEmpty())
  {
    auto it = m_TransformStateStale.GetIterator();
    guid = it.Key();

    auto pAssetInfo = GetAssetInfo(guid);

    // EZ_ASSERT_DEBUG(pAssetInfo != nullptr, "Non-existent assets should not have a tracked transform state.");

    if (pAssetInfo != nullptr)
    {
      out_sAbsPath = pAssetInfo->m_sAbsolutePath;
      return true;
    }
    else
    {
      ezLog::Error("Non-existent assets ('{0}') should not have a tracked transform state.", guid);
      m_TransformStateStale.Remove(it);
    }
  }

  return false;
}

void ezAssetCurator::OnUpdateTaskFinished(ezTask* pTask)
{
  EZ_LOCK(m_CuratorMutex);

  RunNextUpdateTask();
}

void ezAssetCurator::RunNextUpdateTask()
{
  EZ_LOCK(m_CuratorMutex);

  if (!m_bRunUpdateTask || (m_TransformStateStale.IsEmpty() && m_TransformState[ezAssetInfo::TransformState::Unknown].IsEmpty()))
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
// ezAssetCurator Check File System Helper
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::SetAllAssetStatusUnknown()
{
  // tags all known files as unknown, such that we can later remove files
  // that can not be found anymore

  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = ezFileStatus::Status::Unknown;
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
    if (it.Value().m_Status == ezFileStatus::Status::Unknown)
    {
      HandleSingleFile(it.Key());
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
  } while (iterator.Next().Succeeded());
}


void ezAssetCurator::LoadCaches()
{
  EZ_PROFILE("LoadCaches");
  EZ_LOCK(m_CuratorMutex);

  ezStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir);

    ezStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.ezCache");

    ezFileReader reader;
    if (reader.Open(sCacheFile).Succeeded())
    {
      ezUInt32 uiCuratorCacheVersion = 0;
      ezUInt32 uiFileVersion = 0;
      ezUInt32 uiAssetCount = 0;
      ezUInt32 uiFileCount = 0;
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;
      reader >> uiAssetCount;
      reader >> uiFileCount;

      if (uiCuratorCacheVersion != EZ_CURATOR_CACHE_VERSION)
      {
        // Do not purge cache on processors.
        if (!ezQtUiServices::IsHeadless())
        {
          ezStringBuilder sCacheDir = sDataDir;
          sCacheDir.AppendPath("AssetCache");

          QDir dir(sCacheDir.GetData());
          if (dir.exists())
          {
            dir.removeRecursively();
          }
        }
        continue;
      }

      if (uiFileVersion != EZ_CURATOR_CACHE_FILE_VERSION)
        continue;

      const ezRTTI* pFileStatusType = ezGetStaticRTTI<ezFileStatus>();
      {
        EZ_PROFILE("Assets");
        for (ezUInt32 i = 0; i < uiAssetCount; i++)
        {
          ezString sPath;
          reader >> sPath;

          const ezRTTI* pType = nullptr;
          ezAssetDocumentInfo* pEntry = static_cast<ezAssetDocumentInfo*>(ezReflectionSerializer::ReadObjectFromBinary(reader, pType));
          EZ_ASSERT_DEBUG(pEntry != nullptr && pType == ezGetStaticRTTI<ezAssetDocumentInfo>(),
                          "Failed to deserialize ezAssetDocumentInfo!");
          m_CachedAssets.Insert(sPath, ezUniquePtr<ezAssetDocumentInfo>(pEntry, ezFoundation::GetDefaultAllocator()));

          ezFileStatus stat;
          reader >> stat;
          m_CachedFiles.Insert(std::move(sPath), stat);
        }
      }
      {
        EZ_PROFILE("Files");
        for (ezUInt32 i = 0; i < uiFileCount; i++)
        {
          ezString sPath;
          reader >> sPath;
          ezFileStatus stat;
          reader >> stat;
          m_ReferencedFiles.Insert(std::move(sPath), stat);
        }
      }
    }
  }
  ezLog::Debug("Asset Curator LoadCaches: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void ezAssetCurator::SaveCaches()
{
  EZ_PROFILE("SaveCaches");
  m_CachedAssets.Clear();
  m_CachedFiles.Clear();

  // Do not save cache on processors.
  if (ezQtUiServices::IsHeadless())
    return;

  EZ_LOCK(m_CuratorMutex);
  const ezUInt32 uiCuratorCacheVersion = EZ_CURATOR_CACHE_VERSION;

  ezStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir);

    ezStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.ezCache");

    const ezUInt32 uiFileVersion = EZ_CURATOR_CACHE_FILE_VERSION;
    ezUInt32 uiAssetCount = 0;
    ezUInt32 uiFileCount = 0;

    {
      EZ_PROFILE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == ezAssetExistanceState::FileUnchanged && it.Value()->m_sAbsolutePath.StartsWith(sDataDir))
        {
          ++uiAssetCount;
        }
      }
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == ezFileStatus::Status::Valid && !it.Value().m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          ++uiFileCount;
        }
      }
    }
    ezDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;
    writer << uiAssetCount;
    writer << uiFileCount;

    {
      EZ_PROFILE("Assets");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const ezAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == ezAssetExistanceState::FileUnchanged && pAsset->m_sAbsolutePath.StartsWith(sDataDir))
        {
          writer << pAsset->m_sAbsolutePath;
          ezReflectionSerializer::WriteObjectToBinary(writer, ezGetStaticRTTI<ezAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const ezFileStatus* pStat = m_ReferencedFiles.GetValue(it.Value()->m_sAbsolutePath);
          EZ_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      EZ_PROFILE("Files");
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const ezFileStatus& stat = it.Value();
        if (stat.m_Status == ezFileStatus::Status::Valid && !stat.m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    writer.Close();
  }

  ezLog::Debug("Asset Curator SaveCaches: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}
