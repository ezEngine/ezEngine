#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#define EZ_CURATOR_CACHE_VERSION 2
#define EZ_CURATOR_CACHE_FILE_VERSION 6

EZ_IMPLEMENT_SINGLETON(ezAssetCurator);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezAssetCurator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezAssetCurator* pDummy = ezAssetCurator::GetSingleton();
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

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezFileStatus& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezFileStatus)).IgnoreResult();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezFileStatus& ref_value)
{
  inout_stream.ReadBytes(&ref_value, sizeof(ezFileStatus));
  return inout_stream;
}

void ezAssetInfo::Update(ezUniquePtr<ezAssetInfo>& rhs)
{
  // Don't update the existance state, it is handled via ezAssetCurator::SetAssetExistanceState
  // m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pDocumentTypeDescriptor = rhs->m_pDocumentTypeDescriptor;
  m_sAbsolutePath = std::move(rhs->m_sAbsolutePath);
  m_sDataDirParentRelativePath = std::move(rhs->m_sDataDirParentRelativePath);
  m_sDataDirRelativePath = ezStringView(m_sDataDirParentRelativePath.FindSubString("/") + 1); // skip the initial folder
  m_Info = std::move(rhs->m_Info);
  m_AssetHash = rhs->m_AssetHash;
  m_ThumbHash = rhs->m_ThumbHash;
  m_MissingDependencies = rhs->m_MissingDependencies;
  m_MissingReferences = rhs->m_MissingReferences;
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

ezStringView ezSubAsset::GetName() const
{
  if (m_bMainAsset)
    return ezPathUtils::GetFileName(m_pAssetInfo->m_sDataDirParentRelativePath);
  else
    return m_Data.m_sName;
}


void ezSubAsset::GetSubAssetIdentifier(ezStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_sDataDirParentRelativePath;

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
}

ezAssetCurator::~ezAssetCurator()
{
  EZ_ASSERT_DEBUG(m_KnownAssets.IsEmpty(), "Need to call Deinitialize before curator is deleted.");
}

void ezAssetCurator::StartInitialize(const ezApplicationFileSystemConfig& cfg)
{
  EZ_PROFILE_SCOPE("StartInitialize");

  {
    EZ_LOG_BLOCK("SetupAssetProfiles");

    SetupDefaultAssetProfiles();
    if (LoadAssetProfiles().Failed())
    {
      ezLog::Warning("Asset profiles file does not exist or contains invalid data. Setting up default profiles.");
      SaveAssetProfiles().IgnoreResult();
      SaveRuntimeProfiles();
    }
  }

  ComputeAllDocumentManagerAssetProfileHashes();
  BuildFileExtensionSet(m_ValidAssetExtensions);

  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;

  m_pWatcher = EZ_DEFAULT_NEW(ezAssetWatcher, m_FileSystemConfig);
  m_pAssetTableWriter = EZ_DEFAULT_NEW(ezAssetTableWriter, m_FileSystemConfig);

  ezSharedPtr<ezDelegateTask<void>> pInitTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "AssetCuratorUpdateCache", [this]()
    {
    EZ_LOCK(m_CuratorMutex);
    LoadCaches();

    m_CuratorMutex.Unlock();
    CheckFileSystem();
    m_CuratorMutex.Lock();

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
    SaveCaches(); });
  pInitTask->ConfigureTask("Initialize Curator", ezTaskNesting::Never);
  m_InitializeCuratorTaskID = ezTaskSystem::StartSingleTask(pInitTask, ezTaskPriority::FileAccessHighPriority);

  {
    ezAssetCuratorEvent e;
    e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetCurator::WaitForInitialize()
{
  EZ_PROFILE_SCOPE("WaitForInitialize");
  ezTaskSystem::WaitForGroup(m_InitializeCuratorTaskID);
  m_InitializeCuratorTaskID.Invalidate();

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
  EZ_PROFILE_SCOPE("Deinitialize");

  SaveAssetProfiles().IgnoreResult();

  ShutdownUpdateTask();
  ezAssetProcessor::GetSingleton()->StopProcessTask(true);
  m_pWatcher = nullptr;
  m_pAssetTableWriter = nullptr;

  SaveCaches();

  {
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

void ezAssetCurator::MainThreadTick(bool bTopLevel)
{
  CURATOR_PROFILE("MainThreadTick");

  static std::atomic<bool> bReentry = false;
  if (bReentry)
    return;

  if (ezQtEditorApp::GetSingleton()->IsProgressBarProcessingEvents())
    return;

  bReentry = true;

  if (m_pWatcher)
    m_pWatcher->MainThreadTick();

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

  if (bTopLevel && !m_TransformState[ezAssetInfo::TransformState::NeedsImport].IsEmpty())
  {
    const ezUuid assetToImport = *m_TransformState[ezAssetInfo::TransformState::NeedsImport].GetIterator();

    ezAssetInfo* pInfo = GetAssetInfo(assetToImport);

    ProcessAsset(pInfo, nullptr, ezTransformFlags::TriggeredManually);
    UpdateAssetTransformState(assetToImport, ezAssetInfo::TransformState::Unknown);
  }

  if (bTopLevel && m_pAssetTableWriter)
    m_pAssetTableWriter->MainThreadTick();

  bReentry = false;
}

ezDateTime ezAssetCurator::GetLastFullTransformDate() const
{
  ezStringBuilder path = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  ezFileStats stat;
  if (ezOSFile::GetFileStats(path, stat).Failed())
    return {};

  return stat.m_LastModificationTime;
}

void ezAssetCurator::StoreFullTransformDate()
{
  ezStringBuilder path = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  ezOSFile file;
  if (file.Open(path, ezFileOpenMode::Write).Succeeded())
  {
    ezDateTime date;
    date.SetTimestamp(ezTimestamp::CurrentTimestamp());

    path.Format("{}", date);
    file.Write(path.GetData(), path.GetElementCount()).AssertSuccess();
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator High Level Functions
////////////////////////////////////////////////////////////////////////

ezStatus ezAssetCurator::TransformAllAssets(ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile)
{
  EZ_PROFILE_SCOPE("TransformAllAssets");

  ezDynamicArray<ezUuid> assets;
  {
    EZ_LOCK(m_CuratorMutex);
    assets.Reserve(m_KnownAssets.GetCount());
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      assets.PushBack(it.Key());
    }
  }
  ezUInt32 uiNumStepsLeft = assets.GetCount();

  ezUInt32 uiNumFailedSteps = 0;
  ezProgressRange range("Transforming Assets", 1 + uiNumStepsLeft, true);
  for (const ezUuid& assetGuid : assets)
  {
    if (range.WasCanceled())
      break;

    EZ_LOCK(m_CuratorMutex);

    ezAssetInfo* pAssetInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
      continue;

    if (uiNumStepsLeft > 0)
    {
      // it can happen that the number of known assets changes while we are processing them
      // in this case the progress bar may assert that the number of steps completed is larger than
      // what was specified before
      // since this is a valid case, we just stop updating the progress bar, in case more assets are detected

      range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());
      --uiNumStepsLeft;
    }

    ezTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      ezLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  WriteAssetTables(pAssetProfile).IgnoreResult();

  StoreFullTransformDate();

  if (uiNumFailedSteps > 0)
    return ezStatus(ezFmt("Transform all assets failed on {0} assets.", uiNumFailedSteps));

  return ezStatus(EZ_SUCCESS);
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
    range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());

    auto res = ResaveAsset(pAssetInfo);
    if (res.m_Result.Failed())
    {
      ezLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
    }
  }
}

ezTransformStatus ezAssetCurator::TransformAsset(const ezUuid& assetGuid, ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile)
{
  ezTransformStatus res;
  ezStringBuilder sAbsPath;
  ezStopwatch timer;
  const ezAssetDocumentTypeDescriptor* pTypeDesc = nullptr;
  {
    EZ_LOCK(m_CuratorMutex);

    ezAssetInfo* pInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
      return ezTransformStatus("Transform failed, unknown asset.");

    sAbsPath = pInfo->m_sAbsolutePath;
    res = ProcessAsset(pInfo, pAssetProfile, transformFlags);
  }
  if (pTypeDesc && transformFlags.IsAnySet(ezTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument();
    }
  }
  ezLog::Info("Transform asset time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
  return res;
}

ezTransformStatus ezAssetCurator::CreateThumbnail(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return ezStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr, ezTransformFlags::None);
}

ezResult ezAssetCurator::WriteAssetTables(const ezPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  return m_pAssetTableWriter->WriteAssetTables(pAssetProfile, bForce);
}


////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const ezAssetCurator::ezLockedSubAsset ezAssetCurator::FindSubAsset(const char* szPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  EZ_LOCK(m_CuratorMutex);

  if (ezConversionUtils::IsStringUuid(szPathOrGuid))
  {
    return GetSubAsset(ezConversionUtils::ConvertStringToUuid(szPathOrGuid));
  }

  // Split into mainAsset|subAsset
  ezStringBuilder mainAsset;
  ezStringView subAsset;
  const char* szSeparator = ezStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(szPathOrGuid, szSeparator);
    subAsset = ezStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = szPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  ezMap<ezString, ezFileStatus, ezCompareString_NoCase>::ConstIterator it;
  if (ezPathUtils::IsAbsolutePath(mainAsset))
  {
    it = m_ReferencedFiles.Find(mainAsset);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      ezStringBuilder sDataDir;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(mainAsset);
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
        ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
        sDataDir.AppendPath(mainAsset);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  // Did we find an asset?
  if (it.IsValid() && it.Value().m_AssetGuid.IsValid())
  {
    ezAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(it.Value().m_AssetGuid, pAssetInfo);
    EZ_ASSERT_DEV(pAssetInfo != nullptr, "Files reference non-existant assset!");

    if (subAsset.IsValid())
    {
      for (const ezUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto itSub = m_KnownSubAssets.Find(sub);
        if (itSub.IsValid() && subAsset.IsEqual_NoCase(itSub.Value().GetName()))
        {
          return ezLockedSubAsset(m_CuratorMutex, &itSub.Value());
        }
      }
    }
    else
    {
      auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
      return ezLockedSubAsset(m_CuratorMutex, &itSub.Value());
    }
  }

  if (!bExhaustiveSearch)
    return ezLockedSubAsset();

  // TODO: This is the old slow code path that will find the longest substring match.
  // Should be removed or folded into FindBestMatchForFile once it's surely not needed anymore.

  auto FindAsset = [this](ezStringView sPathView) -> ezAssetInfo*
  {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    ezUInt32 uiMinLength = 0xFFFFFFFF;
    ezAssetInfo* pBestInfo = nullptr;

    if (sPathView.IsEmpty())
      return nullptr;

    const ezStringBuilder sPath = sPathView;
    const ezStringBuilder sPathWithSlash("/", sPath);

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (sPathView.IsEqual_NoCase(it.Value()->m_sDataDirParentRelativePath.GetData()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPathWithSlash))
        {
          const ezUInt32 uiLength = it.Value()->m_sDataDirParentRelativePath.GetElementCount();
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

  szSeparator = ezStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    ezStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(szPathOrGuid, szSeparator);

    ezStringView subAsset2(szSeparator + 1);
    if (ezAssetInfo* pAssetInfo = FindAsset(mainAsset2))
    {
      for (const ezUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto subIt = m_KnownSubAssets.Find(sub);
        if (subIt.IsValid() && subAsset2.IsEqual_NoCase(subIt.Value().GetName()))
        {
          return ezLockedSubAsset(m_CuratorMutex, &subIt.Value());
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

const ezAssetCurator::ezLockedAssetTable ezAssetCurator::GetKnownAssets() const
{
  return ezLockedAssetTable(m_CuratorMutex, &m_KnownAssets);
}

ezUInt64 ezAssetCurator::GetAssetDependencyHash(ezUuid assetGuid)
{
  ezUInt64 assetHash = 0;
  ezUInt64 thumbHash = 0;
  ezAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return assetHash;
}

ezUInt64 ezAssetCurator::GetAssetReferenceHash(ezUuid assetGuid)
{
  ezUInt64 assetHash = 0;
  ezUInt64 thumbHash = 0;
  ezAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return thumbHash;
}

void ezAssetCurator::GenerateTransitiveHull(const ezStringView sAssetOrPath, ezSet<ezString>* pDependencies, ezSet<ezString>* pReferences)
{
  if (ezConversionUtils::IsStringUuid(sAssetOrPath))
  {
    auto it = m_KnownSubAssets.Find(ezConversionUtils::ConvertStringToUuid(sAssetOrPath));
    ezAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    const bool bInsertDep = pDependencies && !pDependencies->Contains(pAssetInfo->m_sAbsolutePath);
    const bool bInsertRef = pReferences && !pReferences->Contains(pAssetInfo->m_sAbsolutePath);

    if (bInsertDep)
    {
      pDependencies->Insert(pAssetInfo->m_sAbsolutePath);
    }
    if (bInsertRef)
    {
      pReferences->Insert(pAssetInfo->m_sAbsolutePath);
    }

    if (pDependencies)
    {
      for (const ezString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
      {
        GenerateTransitiveHull(dep, pDependencies, nullptr);
      }
    }

    if (pReferences)
    {
      for (const ezString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
      {
        GenerateTransitiveHull(ref, nullptr, pReferences);
      }
    }
  }
  else
  {
    if (pDependencies && !pDependencies->Contains(sAssetOrPath))
    {
      pDependencies->Insert(sAssetOrPath);
    }
    if (pReferences && !pReferences->Contains(sAssetOrPath))
    {
      pReferences->Insert(sAssetOrPath);
    }
  }
}

ezAssetInfo::TransformState ezAssetCurator::IsAssetUpToDate(const ezUuid& assetGuid, const ezPlatformProfile*, const ezAssetDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_uiAssetHash, ezUInt64& out_uiThumbHash, bool bForce)
{
  return ezAssetCurator::UpdateAssetTransformState(assetGuid, out_uiAssetHash, out_uiThumbHash, bForce);
}

void ezAssetCurator::InvalidateAssetsWithTransformState(ezAssetInfo::TransformState state)
{
  EZ_LOCK(m_CuratorMutex);

  ezHashSet<ezUuid> allWithState = m_TransformState[state];

  for (const auto& asset : allWithState)
  {
    InvalidateAssetTransformState(asset);
  }
}

ezAssetInfo::TransformState ezAssetCurator::UpdateAssetTransformState(ezUuid assetGuid, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("UpdateAssetTransformState");
  {
    EZ_LOCK(m_CuratorMutex);
    // If assetGuid is a sub-asset, redirect to main asset.
    auto it = m_KnownSubAssets.Find(assetGuid);
    if (!it.IsValid())
    {
      return ezAssetInfo::Unknown;
    }
    ezAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    assetGuid = pAssetInfo->m_Info->m_DocumentID;

    // Setting an asset to unknown actually does not change the m_TransformState but merely adds it to the m_TransformStateStale list.
    // This is to prevent the user facing state to constantly fluctuate if something is tagged as modified but not actually changed (E.g. saving a
    // file without modifying the content). Thus we need to check for m_TransformStateStale as well as for the set state.
    if (!bForce && pAssetInfo->m_TransformState != ezAssetInfo::Unknown && !m_TransformStateStale.Contains(assetGuid))
    {
      out_AssetHash = pAssetInfo->m_AssetHash;
      out_ThumbHash = pAssetInfo->m_ThumbHash;
      return pAssetInfo->m_TransformState;
    }
  }
  if (EnsureAssetInfoUpdated(assetGuid).Failed())
  {
    ezStringBuilder tmp;
    ezLog::Error("Asset with GUID {0} is unknown", ezConversionUtils::ToString(assetGuid, tmp));
    return ezAssetInfo::TransformState::Unknown;
  }

  // Data to pull from the asset under the lock that is needed for update computation.
  ezAssetDocumentManager* pManager = nullptr;
  const ezAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  ezString sAssetFile;
  ezUInt8 uiLastStateUpdate = 0;
  ezUInt64 uiSettingsHash = 0;
  ezHybridArray<ezString, 16> assetTransformDependencies;
  ezHybridArray<ezString, 16> runtimeDependencies;
  ezHybridArray<ezString, 16> outputs;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    EZ_LOCK(m_CuratorMutex);
    ezAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);

    pManager = pAssetInfo->GetManager();
    pTypeDescriptor = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile = pAssetInfo->m_sAbsolutePath;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const ezString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
    {
      assetTransformDependencies.PushBack(dep);
    }
    for (const ezString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
    {
      runtimeDependencies.PushBack(ref);
    }
    for (const ezString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
  }

  ezAssetInfo::TransformState state = ezAssetInfo::TransformState::Unknown;
  ezSet<ezString> missingDependencies;
  ezSet<ezString> missingReferences;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, assetTransformDependencies, runtimeDependencies, missingDependencies, missingReferences, out_AssetHash, out_ThumbHash, bForce);
    EZ_ASSERT_DEV(state == ezAssetInfo::Unknown || state == ezAssetInfo::MissingDependency || state == ezAssetInfo::MissingReference, "Unhandled case of HashAsset return value.");

    if (state == ezAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = ezAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(ezAssetDocumentFlags::SupportsThumbnail))
        {
          if (!ezAssetDocumentManager::IsThumbnailUpToDate(sAssetFile, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = ezAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!ezAssetDocumentManager::IsThumbnailUpToDate(sAssetFile, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = ezAssetInfo::TransformState::NeedsTransform;
          }
        }
      }
      else
      {
        state = ezAssetInfo::TransformState::NeedsTransform;
      }
    }
  }

  {
    EZ_LOCK(m_CuratorMutex);
    ezAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (pAssetInfo)
    {
      // Only update the state if the asset state remains unchanged since we gathered its data.
      // Otherwise the state we computed would already be stale. Return the data regardless
      // instead of waiting for a new computation as the case in which the value has actually changed
      // is very rare (asset modified between the two locks) in which case we will just create
      // an already stale transform / thumbnail which will be immediately replaced again.
      if (pAssetInfo->m_LastStateUpdate == uiLastStateUpdate)
      {
        UpdateAssetTransformState(assetGuid, state);
        pAssetInfo->m_AssetHash = out_AssetHash;
        pAssetInfo->m_ThumbHash = out_ThumbHash;
        pAssetInfo->m_MissingDependencies = std::move(missingDependencies);
        pAssetInfo->m_MissingReferences = std::move(missingReferences);
        if (state == ezAssetInfo::TransformState::UpToDate)
        {
          UpdateSubAssets(*pAssetInfo);
        }
      }
    }
    else
    {
      ezStringBuilder tmp;
      ezLog::Error("Asset with GUID {0} is unknown", ezConversionUtils::ToString(assetGuid, tmp));
      return ezAssetInfo::TransformState::Unknown;
    }
    return state;
  }
}

void ezAssetCurator::GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT>& out_count)
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
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  EZ_REPORT_FAILURE("Could not find data directory for asset '{0}", szAbsoluteAssetPath);
  return ezFileSystem::GetSdkRootDirectory();
}

ezResult ezAssetCurator::FindBestMatchForFile(ezStringBuilder& ref_sFile, ezArrayPtr<ezString> allowedFileExtensions) const
{
  // TODO: Merge with exhaustive search in FindSubAsset
  ref_sFile.MakeCleanPath();

  ezStringBuilder testName = ref_sFile;

  for (const auto& ext : allowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (ezFileSystem::ExistsFile(testName))
    {
      ref_sFile = testName;
      goto found;
    }
  }

  testName = ref_sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    ref_sFile = "";
    return EZ_FAILURE;
  }

  if (ezPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    ezPathUtils::MakeValidFilename(testName, '_', ref_sFile);
    return EZ_FAILURE;
  }

  {
    EZ_LOCK(m_CuratorMutex);

    auto SearchFile = [this](ezStringBuilder& ref_sName) -> bool
    {
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status != ezFileStatus::Status::Valid)
          continue;

        const ezString& key = it.Key();

        if (key.EndsWith_NoCase(ref_sName))
        {
          ref_sName = it.Key();
          return true;
        }
      }

      return false;
    };

    // search for the full name
    {
      testName.Prepend("/"); // make sure to not find partial names

      for (const auto& ext : allowedFileExtensions)
      {
        testName.ChangeFileExtension(ext);

        if (SearchFile(testName))
          goto found;
      }
    }

    return EZ_FAILURE;
  }

found:
  if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(testName))
  {
    ref_sFile = testName;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezAssetCurator::FindAllUses(ezUuid assetGuid, ezSet<ezUuid>& ref_uses, bool bTransitive) const
{
  EZ_LOCK(m_CuratorMutex);

  ezSet<ezUuid> todoList;
  todoList.Insert(assetGuid);

  auto GatherReferences = [&](const ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker, const ezStringBuilder& sAsset)
  {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const ezUuid& guid : it.Value())
      {
        if (!ref_uses.Contains(guid))
          todoList.Insert(guid);

        ref_uses.Insert(guid);
      }
    }
  };

  ezStringBuilder sCurrentAsset;
  do
  {
    auto itFirst = todoList.GetIterator();
    const ezAssetInfo* pInfo = GetAssetInfo(itFirst.Key());
    todoList.Remove(itFirst);

    if (pInfo)
    {
      sCurrentAsset = pInfo->m_sAbsolutePath;
      GatherReferences(m_InverseReferences, sCurrentAsset);
      GatherReferences(m_InverseDependency, sCurrentAsset);
    }
  } while (bTransitive && !todoList.IsEmpty());
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::NotifyOfFileChange(const char* szAbsolutePath)
{
  ezStringBuilder sPath(szAbsolutePath);
  sPath.MakeCleanPath();
  HandleSingleFile(sPath);
  // MainThreadTick();
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
  EZ_PROFILE_SCOPE("CheckFileSystem");
  ezStopwatch sw;

  ezProgressRange* range = nullptr;
  if (ezThreadUtils::IsMainThread())
    range = EZ_DEFAULT_NEW(ezProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  EZ_LOCK(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).IgnoreResult();

    if (ezThreadUtils::IsMainThread())
      range->BeginNextStep(dd.m_sDataDirSpecialPath);

    IterateDataDirectory(sTemp);
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

void ezAssetCurator::NeedsReloadResources(const ezUuid& assetGuid)
{
  if (m_pAssetTableWriter)
  {
    m_pAssetTableWriter->NeedsReloadResource(assetGuid);
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Processing
////////////////////////////////////////////////////////////////////////

ezCommandLineOptionEnum opt_AssetThumbnails("_Editor", "-AssetThumbnails", "Whether to generate thumbnails for transformed assets.", "default = 0 | never = 1", 0);

ezTransformStatus ezAssetCurator::ProcessAsset(ezAssetInfo* pAssetInfo, const ezPlatformProfile* pAssetProfile, ezBitflags<ezTransformFlags> transformFlags)
{
  if (transformFlags.IsSet(ezTransformFlags::ForceTransform))
    ezLog::Dev("Asset transform forced.");

  const ezAssetDocumentTypeDescriptor* pTypeDesc = pAssetInfo->m_pDocumentTypeDescriptor;
  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezAssetInfo::TransformState state = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);

  for (const auto& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
  {
    ezBitflags<ezTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(ezTransformFlags::ForceTransform);
    if (ezAssetInfo* pInfo = GetAssetInfo(dep))
    {
      EZ_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  ezTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
  {
    ezBitflags<ezTransformFlags> transformFlagsRefs = transformFlags;
    transformFlagsRefs.Remove(ezTransformFlags::ForceTransform);
    if (ezAssetInfo* pInfo = GetAssetInfo(ref))
    {
      resReferences = ProcessAsset(pInfo, pAssetProfile, transformFlagsRefs);
      if (resReferences.Failed())
        break;
    }
  }


  EZ_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_sDataDirParentRelativePath);

  auto assetFlags = pTypeDesc->m_AssetDocumentFlags;

  // Skip assets that cannot be auto-transformed.
  {
    if (assetFlags.IsAnySet(ezAssetDocumentFlags::DisableTransform))
      return ezStatus(EZ_SUCCESS);

    if (!transformFlags.IsSet(ezTransformFlags::TriggeredManually) && assetFlags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually))
      return ezStatus(EZ_SUCCESS);
  }

  // If references are not complete and we generate thumbnails on transform we can cancel right away.
  if (assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) && resReferences.Failed())
  {
    return resReferences;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  {
    // Sanity check that transforming the dependencies did not change the asset's transform state.
    // In theory this can happen if an asset is transformed by multiple processes at the same time or changes to the file system are being made in the middle of the transform.
    // If this can be reproduced consistently, it is usually a bug in the dependency tracking or other part of the asset curator.
    ezUInt64 uiHash2 = 0;
    ezUInt64 uiThumbHash2 = 0;
    ezAssetInfo::TransformState state2 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash2, uiThumbHash2);

    if (uiHash != uiHash2)
      return ezTransformStatus(ezFmt("Asset hash changed while prosessing dependencies from {} to {}", uiHash, uiHash2));
    if (uiThumbHash != uiThumbHash2)
      return ezTransformStatus(ezFmt("Asset thumbnail hash changed while prosessing dependencies from {} to {}", uiThumbHash, uiThumbHash2));
    if (state != state2)
      return ezTransformStatus(ezFmt("Asset state changed while prosessing dependencies from {} to {}", state, state2));
  }
#endif

  if (transformFlags.IsSet(ezTransformFlags::ForceTransform))
  {
    state = ezAssetInfo::NeedsTransform;
  }

  if (state == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS);

  if (state == ezAssetInfo::TransformState::MissingDependency)
  {
    return ezTransformStatus(ezFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_sAbsolutePath));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezTransformStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

  EZ_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  ezTransformStatus ret;
  ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
  if (state == ezAssetInfo::TransformState::NeedsTransform || (state == ezAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(ezTransformFlags::TriggeredManually) && state == ezAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
    if (ret.Succeeded())
    {
      m_pAssetTableWriter->NeedsReloadResource(pAsset->GetGuid());
    }
  }

  if (state == ezAssetInfo::TransformState::MissingReference)
  {
    return ezTransformStatus(ezFmt("Missing reference for asset '{0}', can't create thumbnail.", pAssetInfo->m_sAbsolutePath));
  }

  if (opt_AssetThumbnails.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified) != 1)
  {
    // skip thumbnail generation, if disabled globally

    if (ret.Succeeded() && assetFlags.IsSet(ezAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) && !resReferences.Failed())
    {
      // If the transformed succeeded, the asset should now be in the NeedsThumbnail state unless the thumbnail already exists in which case we are done or the transform made changes to the asset, e.g. a mesh imported new materials in which case we will revert to transform needed as our dependencies need transform. We simply skip the thumbnail generation in this case.
      ezAssetInfo::TransformState state3 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);
      if (state3 == ezAssetInfo::TransformState::NeedsThumbnail)
      {
        ret = pAsset->CreateThumbnail();
      }
    }
  }

  return ret;
}


ezStatus ezAssetCurator::ResaveAsset(ezAssetInfo* pAssetInfo)
{
  bool bWasOpen = false;
  ezDocument* pDoc = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

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

const ezAssetInfo* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid) const
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

      ezUuid guid0 = pFileStatus->m_AssetGuid;
      if (guid0.IsValid())
      {
        ezAssetInfo* pAssetInfo = m_KnownAssets[guid0];
        UntrackDependencies(pAssetInfo);
        RemoveAssetTransformState(guid0);
        SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileRemoved);
        pFileStatus->m_AssetGuid = ezUuid();
      }

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

    return;
  }
  else
  {
    EZ_ASSERT_DEV(!Stats.m_bIsDirectory, "Directories are handled by ezAssetWatcher and should not pass into this function.");
  }

  HandleSingleFile(sAbsolutePath, Stats);
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath, const ezFileStats& FileStat)
{
  EZ_ASSERT_DEV(!FileStat.m_bIsDirectory, "Directories are handled by ezAssetWatcher and should not pass into this function.");
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

  // Assets should never be in an AssetCache folder.
  const char* szNeedle = sAbsolutePath.FindSubString("AssetCache/");
  if (szNeedle != nullptr && sAbsolutePath.GetData() != szNeedle && szNeedle[-1] == '/')
  {
    return;
  }

  // check that this is an asset type that we know
  if (!m_ValidAssetExtensions.Contains(sExt))
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
  EnsureAssetInfoUpdated(sAbsolutePath).IgnoreResult();
}

void ezAssetCurator::ProcessAllCoreAssets()
{
  EZ_PROFILE_SCOPE("ProcessAllCoreAssets");
  if (ezQtUiServices::IsHeadless())
    return;

  // The 'Core Assets' are always transformed for the PC platform,
  // as they are needed to run the editor properly
  const ezPlatformProfile* pAssetProfile = GetDevelopmentAssetProfile();

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sCoreCollectionPath;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sCoreCollectionPath).IgnoreResult();

    ezStringBuilder sName = sCoreCollectionPath.GetFileName();
    sName.Append(".ezCollectionAsset");
    sCoreCollectionPath.AppendPath(sName);

    QFile coreCollection(sCoreCollectionPath.GetData());
    if (coreCollection.exists())
    {
      auto pSubAsset = FindSubAsset(sCoreCollectionPath);
      if (pSubAsset)
      {
        // prefer certain asset types over others, to ensure that thumbnail generation works
        ezHybridArray<ezTempHashedString, 4> transformOrder;
        transformOrder.PushBack(ezTempHashedString("RenderPipeline"));
        transformOrder.PushBack(ezTempHashedString(""));

        ezTransformStatus resReferences(EZ_SUCCESS);

        for (const ezTempHashedString& name : transformOrder)
        {
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_RuntimeDependencies)
          {
            if (ezAssetInfo* pInfo = GetAssetInfo(ref))
            {
              if (name.GetHash() == 0ull || pInfo->m_Info->m_sAssetsDocumentTypeName == name)
              {
                resReferences = ProcessAsset(pInfo, pAssetProfile, ezTransformFlags::TriggeredManually);
                if (resReferences.Failed())
                  break;
              }
            }
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
    ezTaskSystem::WaitForGroup(m_UpdateTaskGroup);

    EZ_LOCK(m_CuratorMutex);
    m_pUpdateTask.Clear();
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

void ezAssetCurator::OnUpdateTaskFinished(const ezSharedPtr<ezTask>& pTask)
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
    m_pUpdateTask = EZ_DEFAULT_NEW(ezUpdateTask, ezMakeDelegate(&ezAssetCurator::OnUpdateTaskFinished, this));
  }

  if (m_pUpdateTask->IsTaskFinished())
  {
    m_UpdateTaskGroup = ezTaskSystem::StartSingleTask(m_pUpdateTask, ezTaskPriority::FileAccess);
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
    it.Value().m_Status = ezFileStatus::Status::Unknown;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), ezAssetInfo::TransformState::Unknown);
  }
}

void ezAssetCurator::RemoveStaleFileInfos()
{
  ezSet<ezString> unknownFiles;
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    // search for files that existed previously but have not been found anymore recently
    if (it.Value().m_Status == ezFileStatus::Status::Unknown)
    {
      unknownFiles.Insert(it.Key());
    }
  }

  for (const ezString& sFile : unknownFiles)
  {
    HandleSingleFile(sFile);
    m_ReferencedFiles.Remove(sFile);
  }
}

void ezAssetCurator::BuildFileExtensionSet(ezSet<ezString>& AllExtensions)
{
  ezStringBuilder sTemp;
  AllExtensions.Clear();

  const auto& assetTypes = ezAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  ezMap<ezString, const ezDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[ezTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    const auto desc = it.Value();

    if (desc->m_pManager->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
    {
      sTemp = desc->m_sFileExtension;
      sTemp.ToLower();

      AllExtensions.Insert(sTemp);
    }
  }
}

void ezAssetCurator::IterateDataDirectory(const char* szDataDir, ezSet<ezString>* pFoundFiles)
{
  ezStringBuilder sDataDir = szDataDir;
  sDataDir.MakeCleanPath();
  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(szDataDir), "Only absolute paths are supported for directory iteration.");

  while (sDataDir.EndsWith("/"))
    sDataDir.Shrink(0, 1);

  if (sDataDir.IsEmpty())
    return;

  ezFileSystemIterator iterator;
  iterator.StartSearch(sDataDir, ezFileSystemIteratorFlags::ReportFilesRecursive);

  if (!iterator.IsValid())
    return;

  ezStringBuilder sPath;

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();

    HandleSingleFile(sPath, iterator.GetStats());
    if (pFoundFiles)
    {
      pFoundFiles->Insert(sPath);
    }
  }
}

void ezAssetCurator::LoadCaches()
{
  EZ_PROFILE_SCOPE("LoadCaches");
  EZ_LOCK(m_CuratorMutex);

  ezStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

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

      m_KnownAssets.Reserve(m_CachedAssets.GetCount());
      m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

      m_TransformState[ezAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
      m_TransformState[ezAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
      m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
      m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
      m_Updating.Reserve(m_CachedAssets.GetCount());

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
        EZ_PROFILE_SCOPE("Assets");
        for (ezUInt32 i = 0; i < uiAssetCount; i++)
        {
          ezString sPath;
          reader >> sPath;

          const ezRTTI* pType = nullptr;
          ezAssetDocumentInfo* pEntry = static_cast<ezAssetDocumentInfo*>(ezReflectionSerializer::ReadObjectFromBinary(reader, pType));
          EZ_ASSERT_DEBUG(pEntry != nullptr && pType == ezGetStaticRTTI<ezAssetDocumentInfo>(), "Failed to deserialize ezAssetDocumentInfo!");
          m_CachedAssets.Insert(sPath, ezUniquePtr<ezAssetDocumentInfo>(pEntry, ezFoundation::GetDefaultAllocator()));

          ezFileStatus stat;
          reader >> stat;
          m_CachedFiles.Insert(std::move(sPath), stat);
        }
      }
      {
        EZ_PROFILE_SCOPE("Files");
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
  EZ_PROFILE_SCOPE("SaveCaches");
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
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    ezStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.ezCache");

    const ezUInt32 uiFileVersion = EZ_CURATOR_CACHE_FILE_VERSION;
    ezUInt32 uiAssetCount = 0;
    ezUInt32 uiFileCount = 0;

    {
      EZ_PROFILE_SCOPE("Count");
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
      EZ_PROFILE_SCOPE("Assets");
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
      EZ_PROFILE_SCOPE("Files");
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
    writer.Close().IgnoreResult();
  }

  ezLog::Debug("Asset Curator SaveCaches: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void ezAssetCurator::ClearAssetCaches()
{
  const bool bWasRunning = ezAssetProcessor::GetSingleton()->GetProcessTaskState() == ezAssetProcessor::ProcessTaskState::Running;

  if (bWasRunning)
  {
    ezAssetProcessor::GetSingleton()->StopProcessTask(true);
  }

  {
    EZ_LOCK(m_CuratorMutex);

    ezStringBuilder filePath;

    ezSet<ezString> keepAssets;
    ezSet<ezString> filesToDelete;

    for (auto it : m_KnownAssets)
    {
      auto* pAsset = it.Value();
      if (pAsset->GetManager()->GetAssetTypeOutputReliability() != ezAssetDocumentManager::OutputReliability::Perfect)
        continue;

      filePath = pAsset->GetManager()->GetAbsoluteOutputFileName(pAsset->m_pDocumentTypeDescriptor, pAsset->m_sAbsolutePath, nullptr);
      filePath.MakeCleanPath();
      keepAssets.Insert(filePath);
    }

    ezFileSystemIterator iter;
    for (ezFileSystem::StartSearch(iter, "AssetCache/", ezFileSystemIteratorFlags::ReportFilesRecursive); iter.IsValid(); iter.Next())
    {
      iter.GetStats().GetFullPath(filePath);
      filePath.MakeCleanPath();

      if (keepAssets.Contains(filePath))
        continue;

      filesToDelete.Insert(filePath);
    }

    for (const ezString& file : filesToDelete)
    {
      ezOSFile::DeleteFile(file).IgnoreResult();
    }
  }

  if (bWasRunning)
  {
    ezAssetProcessor::GetSingleton()->StartProcessTask();
  }
}
