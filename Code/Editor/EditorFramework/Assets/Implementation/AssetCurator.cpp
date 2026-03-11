#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#define EZ_CURATOR_CACHE_VERSION 2      // Change this to delete and re-gen all asset caches.
#define EZ_CURATOR_CACHE_FILE_VERSION 8 // Change this if for cache format changes.

EZ_IMPLEMENT_SINGLETON(ezAssetCurator);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "FileSystemModel",
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

void ezAssetInfo::Update(ezUniquePtr<ezAssetInfo>& rhs)
{
  // Don't update the existance state, it is handled via ezAssetCurator::SetAssetExistanceState
  // m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pDocumentTypeDescriptor = rhs->m_pDocumentTypeDescriptor;
  m_Path = std::move(rhs->m_Path);
  m_Info = std::move(rhs->m_Info);

  m_AssetHash = rhs->m_AssetHash;
  m_ThumbHash = rhs->m_ThumbHash;
  m_PackageHash = rhs->m_PackageHash;
  m_MissingTransformDeps = std::move(rhs->m_MissingTransformDeps);
  m_MissingThumbnailDeps = std::move(rhs->m_MissingThumbnailDeps);
  m_MissingPackageDeps = std::move(rhs->m_MissingPackageDeps);
  m_CircularDependencies = std::move(rhs->m_CircularDependencies);
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

ezStringView ezSubAsset::GetName() const
{
  if (m_bMainAsset)
    return ezPathUtils::GetFileName(m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
  else
    return m_Data.m_sName;
}


void ezSubAsset::GetSubAssetIdentifier(ezStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_Path.GetDataDirParentRelativePath();

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

  ezFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(ezMakeDelegate(&ezAssetCurator::OnFileChangedEvent, this));
  ezFileSystemModel::FilesMap referencedFiles;
  ezFileSystemModel::FoldersMap referencedFolders;
  LoadCaches(referencedFiles, referencedFolders);
  // We postpone the ezAssetFiles initialize to after we have loaded the cache. No events will be fired before initialize is called.
  ezFileSystemModel::GetSingleton()->Initialize(m_FileSystemConfig, std::move(referencedFiles), std::move(referencedFolders));

  m_pAssetTableWriter = EZ_DEFAULT_NEW(ezAssetTableWriter, m_FileSystemConfig);

  ezSharedPtr<ezDelegateTask<void>> pInitTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "AssetCuratorUpdateCache", ezTaskNesting::Never, [this]()
    {
      EZ_LOCK(m_CuratorMutex);

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

      // Re-save caches after we made a full CheckFileSystem pass.
      ezFileSystemModel::FilesMap referencedFiles;
      ezFileSystemModel::FoldersMap referencedFolders;
      ezFileSystemModel* pFiles = ezFileSystemModel::GetSingleton();
      {
        referencedFiles = *pFiles->GetFiles();
        referencedFolders = *pFiles->GetFolders();
      }
      SaveCaches(referencedFiles, referencedFolders); //
    });
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

  // Broadcast reset.
  {
    ezAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = ezAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
  // Write Asset tables must happen after the reset as that causes the writer to rebuilt its tables.
  WriteAssetTables(nullptr, true).IgnoreResult();
  // This needs to happen after tables are written so that some assets can load their dependencies.
  ProcessAllCoreAssets();
}

void ezAssetCurator::Deinitialize()
{
  EZ_PROFILE_SCOPE("Deinitialize");

  SaveAssetProfiles().IgnoreResult();

  ShutdownUpdateTask();
  ezAssetProcessor::GetSingleton()->StopProcessor(true);
  ezFileSystemModel* pFiles = ezFileSystemModel::GetSingleton();
  ezFileSystemModel::FilesMap referencedFiles;
  ezFileSystemModel::FoldersMap referencedFolders;
  pFiles->Deinitialize(&referencedFiles, &referencedFolders);
  SaveCaches(referencedFiles, referencedFolders);

  pFiles->m_FileChangedEvents.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::OnFileChangedEvent, this));
  pFiles = nullptr;
  m_pAssetTableWriter = nullptr;

  {
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

  ezFileSystemModel::GetSingleton()->MainThreadTick();

  EZ_LOCK(m_CuratorMutex);
  ezTempHybridArray<ezAssetInfo*, 32> deletedAssets;
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
      else if (pInfo->m_ExistanceState == ezAssetExistanceState::FileMoved)
      {
        if (pInfo->m_bMainAsset)
        {
          // Make sure the document knows that its underlying file was renamed.
          if (ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(guid))
            pDoc->DocumentRenamed(pInfo->m_pAssetInfo->m_Path);
        }

        pInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = ezAssetExistanceState::FileUnchanged;
        e.m_Type = ezAssetCuratorEvent::Type::AssetMoved;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == ezAssetExistanceState::FileRemoved)
      {
        // this is a bit tricky:
        // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
        // instead we could set it as modified
        // but then when it was only moved or renamed that means we have another document with the same GUID
        // so once the user would save the now modified document, we would end up with two documents with the same GUID
        // so, for now, since this is probably a rare case anyway, we just close the document without asking
        if (pInfo->m_bMainAsset)
        {
          ezDocumentManager::EnsureDocumentIsClosedInAllManagers(pInfo->m_pAssetInfo->m_Path);
          e.m_Type = ezAssetCuratorEvent::Type::AssetRemoved;
          m_Events.Broadcast(e);

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

  return ezDateTime::MakeFromTimestamp(stat.m_LastModificationTime);
}

void ezAssetCurator::StoreFullTransformDate()
{
  ezStringBuilder path = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  ezOSFile file;
  if (file.Open(path, ezFileOpenMode::Write).Succeeded())
  {
    ezDateTime date;
    date.SetFromTimestamp(ezTimestamp::CurrentTimestamp()).AssertSuccess();

    path.SetFormat("{}", date);
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

      range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));
      --uiNumStepsLeft;
    }

    ezTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      ezLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
  }

  TransformAssetsForSceneExport(pAssetProfile);

  range.BeginNextStep("Writing Lookup Tables");

  WriteAssetTables(pAssetProfile).IgnoreResult();

  StoreFullTransformDate();

  if (uiNumFailedSteps > 0)
    return ezStatus(ezFmt("Transform all assets failed on {0} assets.", uiNumFailedSteps));

  return ezStatus(EZ_SUCCESS);
}

void ezAssetCurator::ResaveAllAssets(ezStringView sPrefixPath)
{
  ezHashTable<ezUuid, ezAssetInfo*> resaveAssets;
  resaveAssets.Reserve(m_KnownAssets.GetCount());

  for (auto itAsset = m_KnownAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    if (ezPathUtils::IsSubPath(sPrefixPath, itAsset.Value()->m_Path.GetAbsolutePath()))
    {
      resaveAssets.Insert(itAsset.Key(), itAsset.Value());
    }
  }

  ezProgressRange range("Re-saving Assets", 1 + resaveAssets.GetCount(), true);

  EZ_LOCK(m_CuratorMutex);

  ezDynamicArray<ezUuid> sortedAssets;
  sortedAssets.Reserve(resaveAssets.GetCount());

  ezMap<ezUuid, ezSet<ezUuid>> dependencies;

  ezSet<ezUuid> accu;

  for (auto itAsset = resaveAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    auto it2 = dependencies.Insert(itAsset.Key(), ezSet<ezUuid>());
    for (const ezString& dep : itAsset.Value()->m_Info->m_TransformDependencies)
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
    range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));

    auto res = ResaveAsset(pAssetInfo);
    if (res.Failed())
    {
      ezLog::Error("{0} ({1})", res.GetMessageString(), pAssetInfo->m_Path.GetDataDirParentRelativePath());
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

    sAbsPath = pInfo->m_Path;
    res = ProcessAsset(pInfo, pAssetProfile, transformFlags);
  }
  if (pTypeDesc && transformFlags.IsAnySet(ezTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument().LogFailure();
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

void ezAssetCurator::TransformAssetsForSceneExport(const ezPlatformProfile* pAssetProfile /*= nullptr*/)
{
  EZ_PROFILE_SCOPE("Transform Special Assets");

  ezSet<ezTempHashedString> types;

  {
    auto& allDMs = ezDocumentManager::GetAllDocumentManagers();
    for (auto& dm : allDMs)
    {
      if (ezAssetDocumentManager* pADM = ezDynamicCast<ezAssetDocumentManager*>(dm))
      {
        pADM->GetAssetTypesRequiringTransformForSceneExport(types);
      }
    }
  }

  ezSet<ezUuid> assets;
  {
    ezAssetCurator::ezLockedAssetTable allAssets = GetKnownAssets();

    for (auto it : *allAssets)
    {
      if (types.Contains(it.Value()->m_Info->m_sAssetsDocumentTypeName))
      {
        assets.Insert(it.Value()->m_Info->m_DocumentID);
      }
    }
  }

  for (const auto& guid : assets)
  {
    // Ignore result
    TransformAsset(guid, ezTransformFlags::TriggeredManually | ezTransformFlags::ForceTransform, pAssetProfile);
  }
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

const ezAssetCurator::ezLockedSubAsset ezAssetCurator::FindSubAsset(ezStringView sPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  EZ_LOCK(m_CuratorMutex);

  if (ezConversionUtils::IsStringUuid(sPathOrGuid))
  {
    return GetSubAsset(ezConversionUtils::ConvertStringToUuid(sPathOrGuid));
  }

  // Split into mainAsset|subAsset
  ezStringBuilder mainAsset;
  ezStringView subAsset;
  const char* szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);
    subAsset = ezStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = sPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  ezFileStatus stat;
  ezResult res = ezFileSystemModel::GetSingleton()->FindFile(mainAsset, stat);

  // Did we find an asset?
  if (res == EZ_SUCCESS && stat.m_DocumentID.IsValid())
  {
    ezAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(stat.m_DocumentID, pAssetInfo);
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
      if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (sPathView.IsEqual_NoCase(it.Value()->m_Path.GetDataDirParentRelativePath()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPathWithSlash))
        {
          const ezUInt32 uiLength = it.Value()->m_Path.GetDataDirParentRelativePath().GetElementCount();
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

  szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    ezStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);

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

  ezStringBuilder sPath = sPathOrGuid;
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

ezUInt64 ezAssetCurator::GetAssetTransformHash(ezUuid assetGuid)
{
  ezUInt64 assetHash = 0;
  ezUInt64 thumbHash = 0;
  ezUInt64 packageHash = 0;
  ezAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, packageHash, false);
  return assetHash;
}

ezUInt64 ezAssetCurator::GetAssetThumbnailHash(ezUuid assetGuid)
{
  ezUInt64 assetHash = 0;
  ezUInt64 packageHash = 0;
  ezUInt64 thumbHash = 0;
  ezAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, packageHash, false);
  return thumbHash;
}

ezAssetInfo::TransformState ezAssetCurator::IsAssetUpToDate(const ezUuid& assetGuid, const ezPlatformProfile*, const ezAssetDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_uiAssetHash, ezUInt64& out_uiThumbHash, ezUInt64& out_uiPackageHash, bool bForce)
{
  if (bForce)
  {
    ezSet<ezUuid> transitiveHull;
    GenerateTransitiveAssetHull(assetGuid, transitiveHull, ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail | ezDependencyFlags::Package);
    // Mark hull as not up to date
    EZ_LOCK(m_CuratorMutex);
    for (auto& asset : transitiveHull)
    {
      InvalidateAssetTransformState(asset);
      // UpdateAssetTransformState(asset, ezAssetInfo::TransformState::Unknown);
    }
  }

  // Running this will update the state of every asset marked as Unknown in the transitive hull.
  return ezAssetCurator::UpdateAssetTransformState(assetGuid, out_uiAssetHash, out_uiThumbHash, out_uiPackageHash, false);
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

ezAssetInfo::TransformState ezAssetCurator::UpdateAssetTransformState(ezUuid assetGuid, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, ezUInt64& out_PackageHash, bool bForce)
{
  CURATOR_PROFILE("UpdateAssetTransformState");
  ezStringBuilder sAbsAssetPath;
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
    sAbsAssetPath = pAssetInfo->m_Path;

    // Circular dependencies can change if any asset in the circle has changed (and potentially broken the circle). Thus, we need to call CheckForCircularDependencies again for every asset.
    if (!pAssetInfo->m_CircularDependencies.IsEmpty() && m_TransformStateStale.Contains(assetGuid))
    {
      pAssetInfo->m_CircularDependencies.Clear();
      if (CheckForCircularDependencies(pAssetInfo).Failed())
      {
        UpdateAssetTransformState(assetGuid, ezAssetInfo::CircularDependency);
        out_AssetHash = 0;
        out_ThumbHash = 0;
        out_PackageHash = 0;
        return ezAssetInfo::CircularDependency;
      }
    }

    // Setting an asset to unknown actually does not change the m_TransformState but merely adds it to the m_TransformStateStale list.
    // This is to prevent the user facing state to constantly fluctuate if something is tagged as modified but not actually changed (E.g. saving a
    // file without modifying the content). Thus we need to check for m_TransformStateStale as well as for the set state.
    if (!bForce && pAssetInfo->m_TransformState != ezAssetInfo::Unknown && !m_TransformStateStale.Contains(assetGuid))
    {
      out_AssetHash = pAssetInfo->m_AssetHash;
      out_ThumbHash = pAssetInfo->m_ThumbHash;
      out_PackageHash = pAssetInfo->m_PackageHash;
      return pAssetInfo->m_TransformState;
    }
  }

  ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsAssetPath);

  // Data to pull from the asset under the lock that is needed for update computation.
  ezAssetDocumentManager* pManager = nullptr;
  const ezAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  ezString sAssetFile;
  ezUInt8 uiLastStateUpdate = 0;
  ezUInt64 uiSettingsHash = 0;
  ezTempHybridArray<ezString, 16> transformDeps;
  ezTempHybridArray<ezString, 16> thumbnailDeps;
  ezTempHybridArray<ezString, 16> packageDeps;
  ezTempHybridArray<ezString, 16> outputs;
  ezTempHybridArray<ezString, 16> subAssetNames;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    EZ_LOCK(m_CuratorMutex);
    ezAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (!pAssetInfo)
    {
      ezStringBuilder tmp;
      ezLog::Error("Asset with GUID {0} is unknown", ezConversionUtils::ToString(assetGuid, tmp));
      return ezAssetInfo::TransformState::Unknown;
    }
    pManager = pAssetInfo->GetManager();
    pTypeDescriptor = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile = pAssetInfo->m_Path;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const ezString& dep : pAssetInfo->m_Info->m_TransformDependencies)
    {
      transformDeps.PushBack(dep);
    }
    for (const ezString& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
    {
      thumbnailDeps.PushBack(ref);
    }
    for (const ezString& ref : pAssetInfo->m_Info->m_PackageDependencies)
    {
      packageDeps.PushBack(ref);
    }
    for (const ezString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
    for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
    {
      if (ezSubAsset* pSubAsset = GetSubAssetInternal(subAssetUuid))
      {
        subAssetNames.PushBack(pSubAsset->m_Data.m_sName);
      }
    }
  }

  ezAssetInfo::TransformState state = ezAssetInfo::TransformState::Unknown;
  ezSet<ezString> missingTransformDeps;
  ezSet<ezString> missingThumbnailDeps;
  ezSet<ezString> missingPackageDeps;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, transformDeps, thumbnailDeps, packageDeps, missingTransformDeps, missingThumbnailDeps, missingPackageDeps, out_AssetHash, out_ThumbHash, out_PackageHash, bForce);
    EZ_ASSERT_DEV(state == ezAssetInfo::Unknown || state == ezAssetInfo::MissingTransformDependency || state == ezAssetInfo::MissingThumbnailDependency || state == ezAssetInfo::MissingPackageDependency, "Unhandled case of HashAsset return value.");

    if (state == ezAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = ezAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(ezAssetDocumentFlags::SupportsThumbnail | ezAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!pManager->IsThumbnailUpToDate(sAssetFile, "", out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) ? ezAssetInfo::TransformState::NeedsTransform : ezAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(ezAssetDocumentFlags::SubAssetsSupportThumbnail | ezAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform))
        {
          for (const ezString& subAssetName : subAssetNames)
          {
            if (!pManager->IsThumbnailUpToDate(sAssetFile, subAssetName, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
            {
              state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(ezAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform) ? ezAssetInfo::TransformState::NeedsTransform : ezAssetInfo::TransformState::NeedsThumbnail;
              break;
            }
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
        pAssetInfo->m_PackageHash = out_PackageHash;
        pAssetInfo->m_MissingTransformDeps = std::move(missingTransformDeps);
        pAssetInfo->m_MissingThumbnailDeps = std::move(missingThumbnailDeps);
        pAssetInfo->m_MissingPackageDeps = std::move(missingPackageDeps);
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

ezString ezAssetCurator::FindDataDirectoryForAsset(ezStringView sAbsoluteAssetPath) const
{
  ezStringBuilder sAssetPath(sAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  EZ_REPORT_FAILURE("Could not find data directory for asset '{0}", sAbsoluteAssetPath);
  return ezFileSystem::GetSdkRootDirectory();
}

void ezAssetCurator::GetAllAssetsInFolder(ezStringView sFolderPath, ezDynamicArray<ezString>& out_assetGuids) const
{
  EZ_LOCK(m_CuratorMutex);

  ezStringBuilder sFolderPathClean = sFolderPath;
  sFolderPathClean.MakeCleanPath();

  if (!sFolderPathClean.EndsWith("/"))
    sFolderPathClean.Append("/");

  ezStringBuilder sTemp;

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    const ezAssetInfo* pAssetInfo = it.Value();
    const ezString& sAssetPath = pAssetInfo->m_Path.GetAbsolutePath();

    sTemp = sAssetPath;
    sTemp.MakeCleanPath();

    if (sTemp.StartsWith_NoCase(sFolderPathClean))
    {
      ezConversionUtils::ToString(pAssetInfo->m_Info->m_DocumentID, sTemp);
      out_assetGuids.PushBack(sTemp);
    }
  }
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
      return ezFileSystemModel::GetSingleton()->FindFile([&ref_sName](const ezDataDirPath& file, const ezFileStatus& stat)
                                                {
                                                  if (stat.m_Status != ezFileStatus::Status::Valid)
                                                    return false;

                                                  if (file.GetAbsolutePath().EndsWith_NoCase(ref_sName))
                                                  {
                                                    ref_sName = file.GetAbsolutePath();
                                                    return true;
                                                  }
                                                  return false; //
                                                })
        .Succeeded();
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
      sCurrentAsset = pInfo->m_Path;
      GatherReferences(m_InverseThumbnailDeps, sCurrentAsset);
      GatherReferences(m_InverseTransformDeps, sCurrentAsset);
    }
  } while (bTransitive && !todoList.IsEmpty());
}

void ezAssetCurator::FindAllUses(ezStringView sAbsolutePath, ezSet<ezUuid>& ref_uses) const
{
  EZ_LOCK(m_CuratorMutex);
  if (auto it = m_InverseTransformDeps.Find(sAbsolutePath); it.IsValid())
  {
    for (const ezUuid& guid : it.Value())
    {
      ref_uses.Insert(guid);
    }
  }
}

bool ezAssetCurator::IsReferenced(ezStringView sAbsolutePath) const
{
  EZ_LOCK(m_CuratorMutex);
  auto it = m_InverseTransformDeps.Find(sAbsolutePath);
  return it.IsValid() && !it.Value().IsEmpty();
}

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void ezAssetCurator::NotifyOfFileChange(ezStringView sAbsolutePath)
{
  ezStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  ezFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
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

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  {
    EZ_LOCK(m_CuratorMutex);
    SetAllAssetStatusUnknown();
  }
  ezFileSystemModel::GetSingleton()->CheckFileSystem();

  if (ezThreadUtils::IsMainThread())
  {
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

void ezAssetCurator::NeedsReloadResources(const ezUuid& assetGuid) const
{
  if (m_pAssetTableWriter)
  {
    m_pAssetTableWriter->NeedsReloadResource(assetGuid);

    ezAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }
}

void ezAssetCurator::GenerateTransitiveHull(const ezStringView sAssetOrPath, ezSet<ezString>& inout_deps, ezBitflags<ezDependencyFlags> dependencyTypes) const
{
  EZ_LOCK(m_CuratorMutex);

  ezTempHybridArray<ezString, 6> toDoList;
  if (ezConversionUtils::IsStringUuid(sAssetOrPath))
  {
    inout_deps.Insert(sAssetOrPath);
    toDoList.PushBack(sAssetOrPath);
  }
  else
  {
    auto subAsset = FindSubAsset(sAssetOrPath);
    if (subAsset.isValid())
    {
      ezStringBuilder sTmp;
      ezConversionUtils::ToString(subAsset->m_pAssetInfo->m_Info->m_DocumentID, sTmp);
      inout_deps.Insert(sTmp);
      toDoList.PushBack(sTmp);
    }
    else
    {
      inout_deps.Insert(sAssetOrPath);
      toDoList.PushBack(sAssetOrPath);
    }
  }

  while (!toDoList.IsEmpty())
  {
    ezString currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (ezConversionUtils::IsStringUuid(currentAsset))
    {
      auto it = m_KnownSubAssets.Find(ezConversionUtils::ConvertStringToUuid(currentAsset));
      ezAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      if (dependencyTypes.IsSet(ezDependencyFlags::Transform))
      {
        for (const ezString& dep : pAssetInfo->m_Info->m_TransformDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (dependencyTypes.IsSet(ezDependencyFlags::Thumbnail))
      {
        for (const ezString& dep : pAssetInfo->m_Info->m_ThumbnailDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (dependencyTypes.IsSet(ezDependencyFlags::Package))
      {
        for (const ezString& dep : pAssetInfo->m_Info->m_PackageDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
    }
  }
}

void ezAssetCurator::GenerateTransitiveAssetHull(const ezUuid& assetGuid, ezSet<ezUuid>& inout_deps, ezBitflags<ezDependencyFlags> dependencyTypes)
{
  ezTempHybridArray<ezUuid, 6> toDoList;

  auto AddDependencies = [&](const ezSet<ezString>& dependencies)
  {
    for (const ezString& dep : dependencies)
    {
      if (!ezConversionUtils::IsStringUuid(dep))
        continue;
      ezUuid guid = ezConversionUtils::ConvertStringToUuid(dep);
      if (!inout_deps.Contains(guid))
      {
        inout_deps.Insert(guid);
        toDoList.PushBack(guid);
      }
    }
  };

  // Build the transitive hull of all assets under 'assetGuid'.
  inout_deps.Insert(assetGuid);
  toDoList.PushBack(assetGuid);
  ezStringBuilder sAbsAssetPath;
  while (!toDoList.IsEmpty())
  {
    ezUuid currentAsset = toDoList.PeekBack();
    toDoList.PopBack();
    {
      EZ_LOCK(m_CuratorMutex);
      auto it = m_KnownSubAssets.Find(currentAsset);
      if (!it.IsValid())
        continue;
      sAbsAssetPath = it.Value().m_pAssetInfo->m_Path;
    }
    // To make sure the dependencies of the asset are up-to-date, we need to check for modifications.
    // This must be done outside the lock to prevent deadlocks.
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsAssetPath);

    EZ_LOCK(m_CuratorMutex);
    auto it = m_KnownSubAssets.Find(currentAsset);
    if (!it.IsValid())
      continue;

    ezAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    if (dependencyTypes.IsSet(ezDependencyFlags::Transform))
      AddDependencies(pAssetInfo->m_Info->m_TransformDependencies);
    if (dependencyTypes.IsSet(ezDependencyFlags::Thumbnail))
      AddDependencies(pAssetInfo->m_Info->m_ThumbnailDependencies);
    if (dependencyTypes.IsSet(ezDependencyFlags::Package))
      AddDependencies(pAssetInfo->m_Info->m_PackageDependencies);
  }
}

void ezAssetCurator::GenerateSettingsHashMap(const ezSet<ezString>& deps, ezBitflags<ezDependencyFlags> dependencyType, ezMap<ezString, ezUInt64>& out_settingsHashMap) const
{
  EZ_LOCK(m_CuratorMutex);

  for (const ezString& sDepOrRef : deps)
  {
    ezUInt64 uiAssetHash = 0;
    if (ezConversionUtils::IsStringUuid(sDepOrRef))
    {
      auto it = m_KnownAssets.Find(ezConversionUtils::ConvertStringToUuid(sDepOrRef));
      if (it.IsValid())
      {
        for (ezDependencyFlags::Enum dep : dependencyType)
        {
          switch (dep)
          {
            case ezDependencyFlags::Thumbnail:
              uiAssetHash = it.Value()->m_ThumbHash;
              break;
            case ezDependencyFlags::Transform:
              uiAssetHash = it.Value()->m_AssetHash;
              break;
            case ezDependencyFlags::Package:
              uiAssetHash = it.Value()->m_PackageHash;
              break;
            default:
              break;
          }
        }
      }
    }
    else
    {
      ezStringBuilder sTmp = sDepOrRef;
      if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTmp))
      {
        ezFileStatus fileStatus;
        ezResult res = ezFileSystemModel::GetSingleton()->HashFile(sTmp, fileStatus);
        uiAssetHash = res.Failed() ? 1 : fileStatus.m_uiHash;
      }
      else
      {
        uiAssetHash = 2;
      }
    }
    out_settingsHashMap.Insert(sDepOrRef, uiAssetHash);
  }
}

void ezAssetCurator::GenerateInverseTransitiveHull(const ezAssetInfo* pAssetInfo, ezSet<ezUuid>& inout_inverseDeps, bool bIncludeTransformDebs, bool bIncludeThumbnailDebs) const
{
  EZ_LOCK(m_CuratorMutex);

  ezTempHybridArray<const ezAssetInfo*, 6> toDoList;
  toDoList.PushBack(pAssetInfo);
  inout_inverseDeps.Insert(pAssetInfo->m_Info->m_DocumentID);

  while (!toDoList.IsEmpty())
  {
    const ezAssetInfo* currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (bIncludeTransformDebs)
    {
      if (auto it = m_InverseTransformDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const ezUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            ezAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }

    if (bIncludeThumbnailDebs)
    {
      if (auto it = m_InverseThumbnailDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const ezUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            ezAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }
  }
}

void ezAssetCurator::WriteDependencyDGML(const ezUuid& guid, ezStringView sOutputFile) const
{
  EZ_LOCK(m_CuratorMutex);

  ezDGMLGraph graph;

  ezSet<ezString> deps;
  ezStringBuilder sTemp;
  GenerateTransitiveHull(ezConversionUtils::ToString(guid, sTemp), deps, ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail);

  ezHashTable<ezString, ezUInt32> nodeMap;
  nodeMap.Reserve(deps.GetCount());
  for (auto& dep : deps)
  {
    ezDGMLGraph::NodeDesc nd;
    if (ezConversionUtils::IsStringUuid(dep))
    {
      auto it = m_KnownSubAssets.Find(ezConversionUtils::ConvertStringToUuid(dep));
      const ezSubAsset& subAsset = it.Value();
      const ezAssetInfo* pAssetInfo = subAsset.m_pAssetInfo;
      if (subAsset.m_bMainAsset)
      {
        nd.m_Color = ezColor::Blue;
        sTemp.SetFormat("{}", pAssetInfo->m_Path.GetDataDirParentRelativePath());
      }
      else
      {
        nd.m_Color = ezColor::AliceBlue;
        sTemp.SetFormat("{} | {}", pAssetInfo->m_Path.GetDataDirParentRelativePath(), subAsset.GetName());
      }
      nd.m_Shape = ezDGMLGraph::NodeShape::Rectangle;
    }
    else
    {
      sTemp = dep;
      nd.m_Color = ezColor::Orange;
      nd.m_Shape = ezDGMLGraph::NodeShape::Rectangle;
    }
    ezUInt32 uiGraphNode = graph.AddNode(sTemp, &nd);
    nodeMap.Insert(dep, uiGraphNode);
  }

  for (auto& node : deps)
  {
    ezDGMLGraph::NodeDesc nd;
    if (ezConversionUtils::IsStringUuid(node))
    {
      ezUInt32 uiInputNode = *nodeMap.GetValue(node);

      auto it = m_KnownSubAssets.Find(ezConversionUtils::ConvertStringToUuid(node));
      ezAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      ezMap<ezUInt32, ezString> connection;

      auto ExtendConnection = [&](const ezString& sRef, ezStringView sLabel)
      {
        ezUInt32 uiOutputNode = *nodeMap.GetValue(sRef);
        sTemp = connection[uiOutputNode];
        if (sTemp.IsEmpty())
          sTemp = sLabel;
        else
          sTemp.AppendFormat(" | {}", sLabel);
        connection[uiOutputNode] = sTemp;
      };

      for (const ezString& sRef : pAssetInfo->m_Info->m_TransformDependencies)
      {
        ExtendConnection(sRef, "Transform");
      }

      for (const ezString& sRef : pAssetInfo->m_Info->m_ThumbnailDependencies)
      {
        ExtendConnection(sRef, "Thumbnail");
      }

      // This will make the graph very big, not recommended.
      /* for (const ezString& ref : pAssetInfo->m_Info->m_PackageDependencies)
       {
         ExtendConnection(ref, "Package");
       }*/

      for (auto it : connection)
      {
        graph.AddConnection(uiInputNode, it.Key(), it.Value());
      }
    }
  }

  ezDGMLGraphWriter::WriteGraphToFile(sOutputFile, graph).IgnoreResult();
}

ezAssetCurator::ExportResult ezAssetCurator::ExportAssets(ezArrayPtr<ezString> sources, ezStringView sDestinationFolder, ezBitflags<ezDependencyFlags> includeDependencyTypes) const
{
  EZ_LOCK(m_CuratorMutex);

  ExportResult result;

  ezSet<ezString> allDependencies;

  for (const ezString& source : sources)
  {
    GenerateTransitiveHull(source, allDependencies, includeDependencyTypes);
  }

  ezStringBuilder sDestPath = sDestinationFolder;
  ezStringBuilder sAbsPath;
  ezStringBuilder sRelPath;
  ezStringBuilder sTargetPath;
  ezStringBuilder sTargetDir;

  for (const ezString& dep : allDependencies)
  {
    sAbsPath.Clear();
    const ezDataDirectoryInfo* ddi = nullptr;

    if (ezConversionUtils::IsStringUuid(dep))
    {
      ezUuid depGuid = ezConversionUtils::ConvertStringToUuid(dep);
      const auto pDepAsset = GetSubAsset(depGuid);

      if (!pDepAsset.isValid())
        continue;

      sAbsPath = pDepAsset->m_pAssetInfo->m_Path.GetAbsolutePath();
      sRelPath = pDepAsset->m_pAssetInfo->m_Path.GetDataDirRelativePath();

      // the index in GetDataDirIndex() doesn't seem to match the index of ezFileSystem
      // ddi = &ezFileSystem::GetDataDirectoryInfo(pDepAsset->m_pAssetInfo->m_Path.GetDataDirIndex());

      if (ezFileSystem::ResolvePath(sAbsPath, &sAbsPath, &sRelPath, &ddi).Failed())
        continue;
    }
    else
    {
      if (ezFileSystem::ResolvePath(dep, &sAbsPath, &sRelPath, &ddi).Failed())
        continue;
    }

    if (ddi->m_sRootName == "BASE")
      continue;

    if (!ezOSFile::ExistsFile(sAbsPath))
      continue;


    ezStringBuilder sTargetPath = sDestPath;
    sTargetPath.AppendPath(sRelPath);

    if (ezOSFile::CopyFile(sAbsPath, sTargetPath).Failed())
    {
      result.m_uiFailedFiles++;
      continue;
    }

    ++result.m_uiCopiedFiles;
  }

  return result;
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
  ezUInt64 uiPackageHash = 0;
  ezAssetInfo::TransformState state = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash, uiPackageHash);

  if (state == ezAssetInfo::TransformState::CircularDependency)
  {
    return ezTransformStatus(ezFmt("Circular dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  for (const auto& dep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    ezBitflags<ezTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(ezTransformFlags::ForceTransform);
    if (ezAssetInfo* pInfo = GetAssetInfo(dep))
    {
      EZ_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  ezTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
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


  EZ_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_Path.GetDataDirParentRelativePath());

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
    ezUInt64 uiPackageHash2 = 0;
    ezAssetInfo::TransformState state2 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash2, uiThumbHash2, uiPackageHash2);

    if (uiHash != uiHash2)
      return ezTransformStatus(ezFmt("Asset hash changed while processing dependencies from {} to {}", uiHash, uiHash2));
    if (uiThumbHash != uiThumbHash2)
      return ezTransformStatus(ezFmt("Asset thumbnail hash changed while processing dependencies from {} to {}", uiThumbHash, uiThumbHash2));
    if (uiPackageHash != uiPackageHash2)
      return ezTransformStatus(ezFmt("Asset package hash changed while processing dependencies from {} to {}", uiPackageHash, uiPackageHash2));
    if (state != state2)
      return ezTransformStatus(ezFmt("Asset state changed while processing dependencies from {} to {}", state, state2));
  }
#endif

  if (transformFlags.IsSet(ezTransformFlags::ForceTransform))
  {
    state = ezAssetInfo::NeedsTransform;
  }

  if (state == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS);

  if (state == ezAssetInfo::TransformState::MissingTransformDependency)
  {
    return ezTransformStatus(ezFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  ezDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezTransformStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

  EZ_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  ezTransformStatus ret;
  ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
  if (state == ezAssetInfo::TransformState::NeedsTransform || (state == ezAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(ezTransformFlags::TriggeredManually) && state == ezAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
    if (ret.Succeeded())
    {
      m_pAssetTableWriter->NeedsReloadResource(pAsset->GetGuid());

      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }

  if (state == ezAssetInfo::TransformState::MissingPackageDependency)
  {
    return ezTransformStatus(ezFmt("Missing package dependency for asset '{0}'. Asset compromised.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  if (state == ezAssetInfo::TransformState::MissingThumbnailDependency)
  {
    return ezTransformStatus(ezFmt("Missing thumbnail dependency for asset '{0}', can't create thumbnail.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  if (opt_AssetThumbnails.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified) != 1)
  {
    // skip thumbnail generation, if disabled globally

    if (ret.Succeeded() && assetFlags.IsSet(ezAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(ezAssetDocumentFlags::AutoThumbnailOnTransform) && !resReferences.Failed())
    {
      // If the transformed succeeded, the asset should now be in the NeedsThumbnail state unless the thumbnail already exists in which case we are done or the transform made changes to the asset, e.g. a mesh imported new materials in which case we will revert to transform needed as our dependencies need transform. We simply skip the thumbnail generation in this case.
      ezAssetInfo::TransformState state3 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash, uiPackageHash);
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
  ezDocument* pDoc = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

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

void ezAssetCurator::OnFileChangedEvent(const ezFileChangedEvent& e)
{
  switch (e.m_Type)
  {
    case ezFileChangedEvent::Type::DocumentLinked:
    case ezFileChangedEvent::Type::DocumentUnlinked:
      break;
    case ezFileChangedEvent::Type::FileAdded:
    case ezFileChangedEvent::Type::FileChanged:
    {
      // If the asset was just added it is not tracked and thus no need to invalidate anything.
      if (e.m_Type == ezFileChangedEvent::Type::FileChanged)
      {
        EZ_LOCK(m_CuratorMutex);
        ezUuid guid0 = e.m_Status.m_DocumentID;
        if (guid0.IsValid())
          InvalidateAssetTransformState(guid0);

        auto it = m_InverseTransformDeps.Find(e.m_Path);
        if (it.IsValid())
        {
          for (const ezUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
        if (it2.IsValid())
        {
          for (const ezUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }
      }

      // Assets should never be in an AssetCache folder.
      if (e.m_Path.GetAbsolutePath().FindSubString("/AssetCache/") != nullptr)
      {
        return;
      }

      // check that this is an asset type that we know
      ezStringBuilder sExt = ezPathUtils::GetFileExtension(e.m_Path);
      sExt.ToLower();
      if (!m_ValidAssetExtensions.Contains(sExt))
      {
        return;
      }

      EnsureAssetInfoUpdated(e.m_Path, e.m_Status).IgnoreResult();
    }
    break;
    case ezFileChangedEvent::Type::FileRemoved:
    {
      EZ_LOCK(m_CuratorMutex);
      ezUuid guid0 = e.m_Status.m_DocumentID;
      if (guid0.IsValid())
      {
        if (auto it = m_KnownAssets.Find(guid0); it.IsValid())
        {
          ezAssetInfo* pAssetInfo = it.Value();
          EZ_ASSERT_DEBUG(ezFileSystemModel::IsSameFile(e.m_Path, pAssetInfo->m_Path), "");
          UntrackDependencies(pAssetInfo);
          RemoveAssetTransformState(guid0);
          SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileRemoved);
        }
      }
      auto it = m_InverseTransformDeps.Find(e.m_Path);
      if (it.IsValid())
      {
        for (const ezUuid& guid : it.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }

      auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
      if (it2.IsValid())
      {
        for (const ezUuid& guid : it2.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }
    }
    break;
    case ezFileChangedEvent::Type::ModelReset:
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
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
        ezTempHybridArray<ezTempHashedString, 4> transformOrder;
        transformOrder.PushBack(ezTempHashedString("RenderPipeline"));
        transformOrder.PushBack(ezTempHashedString());

        ezTransformStatus resReferences(EZ_SUCCESS);

        for (const ezTempHashedString& name : transformOrder)
        {
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_PackageDependencies)
          {
            if (ezAssetInfo* pInfo = GetAssetInfo(ref))
            {
              if (name == ezTempHashedString() || pInfo->m_Info->m_sAssetsDocumentTypeName == name)
              {
                resReferences = ProcessAsset(pInfo, pAssetProfile, ezTransformFlags::TriggeredManually);
                if (resReferences.Failed())
                {
                  ezLog::Error("Core asset '{}' of type '{}' failed transformation.", ref, pInfo->m_Info->m_sAssetsDocumentTypeName);
                }
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
      out_sAbsPath = pAssetInfo->m_Path;
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

  if (ezQtEditorApp::GetSingleton()->IsInHeadlessMode())
    return;

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
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), ezAssetInfo::TransformState::Unknown);
  }
}

void ezAssetCurator::LoadCaches(ezFileSystemModel::FilesMap& out_referencedFiles, ezFileSystemModel::FoldersMap& out_referencedFolders)
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
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;

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

      {
        EZ_PROFILE_SCOPE("Assets");
        ezUInt32 uiAssetCount = 0;
        reader >> uiAssetCount;
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

        m_KnownAssets.Reserve(m_CachedAssets.GetCount());
        m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

        m_TransformState[ezAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
        m_TransformState[ezAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
        m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
        m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
        m_Updating.Reserve(m_CachedAssets.GetCount());
      }
      {
        EZ_PROFILE_SCOPE("Files");
        ezUInt32 uiFileCount = 0;
        reader >> uiFileCount;
        for (ezUInt32 i = 0; i < uiFileCount; i++)
        {
          ezDataDirPath path;
          reader >> path;
          ezFileStatus stat;
          reader >> stat;
          // We invalidate all asset guids as the current cache as stored on disk is missing various bits in the curator that requires the code to go through the found new asset init code on load again.
          stat.m_DocumentID = ezUuid::MakeInvalid();
          out_referencedFiles.Insert(std::move(path), stat);
        }
      }

      {
        EZ_PROFILE_SCOPE("Folders");
        ezUInt32 uiFolderCount = 0;
        reader >> uiFolderCount;
        for (ezUInt32 i = 0; i < uiFolderCount; i++)
        {
          ezDataDirPath path;
          reader >> path;
          ezFileStatus::Status stat;
          reader >> (ezUInt8&)stat;
          out_referencedFolders.Insert(std::move(path), stat);
        }
      }
    }
  }

  ezLog::Debug("Asset Curator LoadCaches: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void ezAssetCurator::SaveCaches(const ezFileSystemModel::FilesMap& referencedFiles, const ezFileSystemModel::FoldersMap& referencedFolders)
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
  for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i];

    ezStringBuilder sDataDir;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    ezStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.ezCache");

    const ezUInt32 uiFileVersion = EZ_CURATOR_CACHE_FILE_VERSION;
    ezUInt32 uiAssetCount = 0;
    ezUInt32 uiFileCount = 0;
    ezUInt32 uiFolderCount = 0;

    {
      EZ_PROFILE_SCOPE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == ezAssetExistanceState::FileUnchanged && it.Value()->m_Path.GetDataDirIndex() == i)
        {
          ++uiAssetCount;
        }
      }
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == ezFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFileCount;
        }
      }
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value() == ezFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFolderCount;
        }
      }
    }
    ezDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;

    {
      EZ_PROFILE_SCOPE("Assets");
      writer << uiAssetCount;
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const ezAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == ezAssetExistanceState::FileUnchanged && pAsset->m_Path.GetDataDirIndex() == i)
        {
          writer << pAsset->m_Path.GetAbsolutePath();
          ezReflectionSerializer::WriteObjectToBinary(writer, ezGetStaticRTTI<ezAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const ezFileStatus* pStat = referencedFiles.GetValue(it.Value()->m_Path);
          EZ_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      EZ_PROFILE_SCOPE("Files");
      writer << uiFileCount;
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const ezFileStatus& stat = it.Value();
        if (stat.m_Status == ezFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    {
      EZ_PROFILE_SCOPE("Folders");
      writer << uiFolderCount;
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        const ezFileStatus::Status stat = it.Value();
        if (stat == ezFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << (ezUInt8)stat;
        }
      }
    }

    writer.Close().IgnoreResult();
  }

  ezLog::Debug("Asset Curator SaveCaches: {0} ms", ezArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void ezAssetCurator::ClearAssetCaches(ezAssetDocumentManager::OutputReliability threshold)
{
  const bool bWasRunning = ezAssetProcessor::GetSingleton()->GetProcessorState() == ezAssetProcessor::ProcessorState::Running;

  if (bWasRunning)
  {
    // pause background asset processing while we delete files
    ezAssetProcessor::GetSingleton()->StopProcessor(true);
  }

  {
    EZ_LOCK(m_CuratorMutex);

    ezStringBuilder filePath;

    ezSet<ezString> keepAssets;
    ezSet<ezString> filesToDelete;

    // for all assets, gather their outputs and check which ones we want to keep
    // e.g. textures are perfectly reliable, and even when clearing the cache we can keep them, also because they cost a lot of time to regenerate
    for (auto it : m_KnownSubAssets)
    {
      const auto& subAsset = it.Value();
      auto pManager = subAsset.m_pAssetInfo->GetManager();
      if (pManager->GetAssetTypeOutputReliability() > threshold)
      {
        auto pDocumentTypeDescriptor = subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor;
        const auto& path = subAsset.m_pAssetInfo->m_Path;

        // check additional outputs
        for (const auto& output : subAsset.m_pAssetInfo->m_Info->m_Outputs)
        {
          filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, output);
          filePath.MakeCleanPath();
          keepAssets.Insert(filePath);
        }

        filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, nullptr);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);

        // and also keep the thumbnail
        filePath = pManager->GenerateResourceThumbnailPath(path, subAsset.m_Data.m_sName);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);
      }
    }

    // iterate over all AssetCache folders in all data directories and gather the list of files for deletion
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

  ezAssetCurator::CheckFileSystem();

  ezAssetCurator::ProcessAllCoreAssets();

  if (bWasRunning)
  {
    // restart background asset processing
    ezAssetProcessor::GetSingleton()->StartProcessor();
  }
}

ezUInt32 ezAssetCurator::ReplaceAssetReferenceInObject(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, ezStringView sOldReference, ezStringView sNewReference, ezDynamicArray<ezString>& out_errors)
{
  ezUInt32 uiReplacementCount = 0;

  const ezRTTI* pType = pObject->GetTypeAccessor().GetType();
  ezTempHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (const ezAbstractProperty* pProp : properties)
  {
    // Skip temporary properties
    if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    // Check if this is an asset reference property
    const ezAssetBrowserAttribute* pAssetAttr = pProp->GetAttributeByType<ezAssetBrowserAttribute>();
    if (pAssetAttr == nullptr)
      continue;

    // Must be string type
    const auto propVarType = pProp->GetSpecificType()->GetVariantType();
    if (propVarType != ezVariantType::String && propVarType != ezVariantType::StringView)
      continue;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          ezVariant value;
          if (pAccessor->GetValue(pObject, pProp, value).Succeeded())
          {
            ezString sValue = value.Get<ezString>();
            if (sValue == sOldReference)
            {
              if (pAccessor->SetValue(pObject, pProp, ezVariant(ezString(sNewReference))).Succeeded())
              {
                uiReplacementCount++;
              }
              else
              {
                ezStringBuilder sError;
                sError.SetFormat("Failed to replace property '{}'", pProp->GetPropertyName());
                out_errors.PushBack(sError);
              }
            }
          }
        }
      }
      break;

      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          ezInt32 iCount = pAccessor->GetCount(pObject, pProp);

          for (ezInt32 i = 0; i < iCount; ++i)
          {
            ezVariant value;
            if (pAccessor->GetValue(pObject, pProp, value, i).Succeeded())
            {
              ezString sValue = value.Get<ezString>();
              if (sValue == sOldReference)
              {
                if (pAccessor->SetValue(pObject, pProp, ezVariant(ezString(sNewReference)), i).Succeeded())
                {
                  uiReplacementCount++;
                }
                else
                {
                  ezStringBuilder sError;
                  sError.SetFormat("Failed to replace property '{}[{}]'", pProp->GetPropertyName(), i);
                  out_errors.PushBack(sError);
                }
              }
            }
          }
        }
      }
      break;

      case ezPropertyCategory::Map:
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          ezDynamicArray<ezVariant> keys;
          if (pAccessor->GetKeys(pObject, pProp, keys).Succeeded())
          {
            for (const ezVariant& key : keys)
            {
              ezVariant value;
              if (pAccessor->GetValue(pObject, pProp, value, key).Succeeded())
              {
                ezString sValue = value.Get<ezString>();
                if (sValue == sOldReference)
                {
                  if (pAccessor->SetValue(pObject, pProp, ezVariant(ezString(sNewReference)), key).Succeeded())
                  {
                    uiReplacementCount++;
                  }
                  else
                  {
                    ezStringBuilder sError;
                    sError.SetFormat("Failed to replace map property '{}[{}]'", pProp->GetPropertyName(), key.ConvertTo<ezString>());
                    out_errors.PushBack(sError);
                  }
                }
              }
            }
          }
        }
      }
      break;

      default:
        break;
    }
  }

  // Process children recursively
  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType() != nullptr &&
        pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;
    uiReplacementCount += ReplaceAssetReferenceInObject(pAccessor, pChild, sOldReference, sNewReference, out_errors);
  }

  return uiReplacementCount;
}

ezUInt32 ezAssetCurator::ReplaceAssetReferenceInDocument(ezDocument* pDocument, ezStringView sOldReference, ezStringView sNewReference, ezDynamicArray<ezString>& out_errors)
{
  ezObjectAccessorBase* pAccessor = pDocument->GetObjectAccessor();

  pAccessor->StartTransaction("Replace Asset Reference");

  ezUInt32 uiReplacementCount = ReplaceAssetReferenceInObject(
    pAccessor,
    pDocument->GetObjectManager()->GetRootObject(),
    sOldReference,
    sNewReference,
    out_errors);

  if (uiReplacementCount > 0)
    pAccessor->FinishTransaction();
  else
    pAccessor->CancelTransaction();

  return uiReplacementCount;
}

ezAssetCurator::ReplaceAssetResult ezAssetCurator::ReplaceAssetReferenceInUses(ezUuid assetToReplace, ezStringView sOldReference, ezStringView sNewReference)
{
  ReplaceAssetResult result;

  // Find all direct uses of this asset
  ezSet<ezUuid> uses;
  ezAssetCurator::GetSingleton()->FindAllUses(assetToReplace, uses, false /* bTransitive */);

  for (const ezUuid& useGuid : uses)
  {
    // Get the asset info to find the document path
    const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(useGuid);
    if (!pSubAsset.isValid())
    {
      result.m_Errors.PushBack("Could not find asset info for a referencing asset");
      result.m_uiDocumentsFailed++;
      continue;
    }

    ezString sDocumentPath = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

    // Open the document (without requesting a window)
    ezDocument* pDocument = ezQtEditorApp::GetSingleton()->OpenDocument(sDocumentPath, ezDocumentFlags::None);

    if (pDocument == nullptr)
    {
      ezStringBuilder sError;
      sError.SetFormat("Failed to open document: {}", sDocumentPath);
      result.m_Errors.PushBack(sError);
      result.m_uiDocumentsFailed++;
      continue;
    }

    ezDynamicArray<ezString> docErrors;
    ezUInt32 uiReplaced = ReplaceAssetReferenceInDocument(pDocument, sOldReference, sNewReference, docErrors);

    result.m_Errors.PushBackRange(docErrors);

    if (uiReplaced > 0)
    {
      // Save the document
      ezStatus saveStatus = pDocument->SaveDocument(false);
      if (saveStatus.Failed())
      {
        ezStringBuilder sError;
        sError.SetFormat("Failed to save document: {} - {}", sDocumentPath, saveStatus.GetMessageString());
        result.m_Errors.PushBack(sError);
        result.m_uiDocumentsFailed++;
      }
      else
      {
        result.m_uiDocumentsModified++;
        result.m_uiPropertiesReplaced += uiReplaced;
      }
    }
  }

  return result;
}
