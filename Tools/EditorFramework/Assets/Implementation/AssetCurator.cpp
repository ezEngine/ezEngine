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

EZ_IMPLEMENT_SINGLETON(ezAssetCurator);

ezAssetCurator::ezAssetCurator()
  : m_SingletonRegistrar(this)
{
  m_bRunUpdateTask = false;
  m_pUpdateTask = nullptr;
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
}

ezAssetCurator::~ezAssetCurator()
{
  Deinitialize();

  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
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


void ezAssetCurator::SetActivePlatform(const char* szPlatform)
{
  m_sActivePlatform = szPlatform;

  ezAssetCuratorEvent e;
  e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;

  m_Events.Broadcast(e);
}

void ezAssetCurator::NotifyOfPotentialAsset(const char* szAbsolutePath)
{
  HandleSingleFile(szAbsolutePath);

  /// \todo Move this into a regularly running thread or so
  MainThreadTick();
}


bool ezAssetCurator::IsAssetUpToDate(const ezUuid& assetGuid, const char* szPlatform, const ezDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_AssetHash)
{
  out_AssetHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(assetGuid);

  if (out_AssetHash == 0)
    return false;

  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  const ezString sTargetFile = static_cast<ezAssetDocumentManager*>(pTypeDescriptor->m_pManager)->GetFinalOutputFileName(pTypeDescriptor, GetAssetInfo(assetGuid)->m_sAbsolutePath, sPlatform);

  if (ezAssetDocumentManager::IsResourceUpToDate(out_AssetHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion(), sTargetFile))
  {
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::UpToDate);
    return true;
  }
  else
  {
    UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::NeedsTransform);
    return false;
  }
}


void ezAssetCurator::UpdateAssetTransformState(const ezUuid& assetGuid, ezAssetInfo::TransformState state)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    if (pAssetInfo->m_TransformState != state)
    {
      pAssetInfo->m_TransformState = state;
      m_TransformStateChanged.Insert(assetGuid);
    }

    switch (state)
    {
    case ezAssetInfo::TransformState::Unknown:
      m_TransformStateUnknown.Insert(assetGuid);
      m_TransformStateNeedsTransform.Remove(assetGuid);
      m_TransformStateNeedsThumbnail.Remove(assetGuid);
      break;
    case ezAssetInfo::TransformState::NeedsTransform:
      m_TransformStateNeedsTransform.Insert(assetGuid);
      break;
    case ezAssetInfo::TransformState::NeedsThumbnail:
      m_TransformStateNeedsThumbnail.Insert(assetGuid);
      break;
    }
  }
}


void ezAssetCurator::GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezUInt32& out_uiNumUnknown, ezUInt32& out_uiNumNeedTransform, ezUInt32& out_uiNumNeedThumb)
{
  EZ_LOCK(m_CuratorMutex);

  out_uiNumAssets = m_KnownAssets.GetCount();
  out_uiNumUnknown = m_TransformStateUnknown.GetCount();
  out_uiNumNeedTransform = m_TransformStateNeedsTransform.GetCount();
  out_uiNumNeedThumb = m_TransformStateNeedsThumbnail.GetCount();
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath)
{
  ezFileStats Stats;
  if (ezOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
    return;

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

  // mark the file as valid (ie. we saw it on disk, so it hasn't been deleted or such)
  RefFile.m_Status = FileStatus::Status::Valid;

  // if the time stamp changed, reset the hash, such that the file will be hashed again later
  if (!RefFile.m_Timestamp.IsEqual(FileStat.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
  {
    RefFile.m_Timestamp = FileStat.m_LastModificationTime;
    RefFile.m_uiHash = 0;
  }

  // check that this is an asset type that we know
  if (!validExtensions.Contains(sExt))
    return;

  // the file is a known asset type
  // so make sure it gets a valid GUID assigned

  if (RefFile.m_AssetGuid.IsValid() && RefFile.m_uiHash != 0)
    return;

  // store the folder of the asset
  {
    ezStringBuilder sAssetFolder = sAbsolutePath;
    sAssetFolder = sAssetFolder.GetFileDirectory();

    m_AssetFolders.Insert(sAssetFolder);
  }

  // Find Asset Info
  ezAssetInfo* pAssetInfo = nullptr;
  ezUuid oldGuid = RefFile.m_AssetGuid;
  bool bNew = false;

  // if it already has a valid GUID, an ezAssetInfo object must exist
  if (RefFile.m_AssetGuid.IsValid())
  {
    EZ_VERIFY(m_KnownAssets.TryGetValue(RefFile.m_AssetGuid, pAssetInfo), "guid set in file-status but no asset is actually known under that guid");
  }
  else
  {
    // otherwise create a new ezAssetInfo object

    bNew = true;
    pAssetInfo = EZ_DEFAULT_NEW(ezAssetInfo);
  }

  // this will update m_AssetGuid by reading it from the asset JSON file
  UpdateAssetInfo(sAbsolutePath, RefFile, *pAssetInfo, &FileStat); // ignore return value

  if (bNew)
  {
    // now the GUID must be valid
    EZ_ASSERT_DEV(pAssetInfo->m_Info.m_DocumentID.IsValid(), "Asset header read for '%s', but its GUID is invalid! Corrupted document?", sAbsolutePath.GetData());
    EZ_ASSERT_DEV(RefFile.m_AssetGuid == pAssetInfo->m_Info.m_DocumentID, "UpdateAssetInfo broke the GUID!");
    // and we can store the new ezAssetInfo data under that GUID
    EZ_ASSERT_DEV(!m_KnownAssets.Contains(pAssetInfo->m_Info.m_DocumentID), "The assets '%s' and '%s' share the same GUID!", pAssetInfo->m_sAbsolutePath.GetData(), m_KnownAssets[pAssetInfo->m_Info.m_DocumentID]->m_sAbsolutePath.GetData());
    m_KnownAssets[pAssetInfo->m_Info.m_DocumentID] = pAssetInfo;
  }
  else
  {
    // in case the previous GUID is different to what UpdateAssetInfo read from file, get rid of the old GUID
    // and point the new one to the existing ezAssetInfo
    if (oldGuid != RefFile.m_AssetGuid)
    {
      m_KnownAssets.Remove(oldGuid);
      m_KnownAssets.Insert(RefFile.m_AssetGuid, pAssetInfo);
    }
  }

  // asset changed or is new -> check asset transform state
  pAssetInfo->m_TransformState = ezAssetInfo::TransformState::Unknown;
  m_TransformStateUnknown.Insert(RefFile.m_AssetGuid);
  m_TransformStateNeedsTransform.Remove(RefFile.m_AssetGuid);
  m_TransformStateNeedsThumbnail.Remove(RefFile.m_AssetGuid);
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

void ezAssetCurator::Initialize(const ezApplicationFileSystemConfig& cfg)
{
  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;
  m_sActivePlatform = "PC";

  CheckFileSystem();
}

void ezAssetCurator::Deinitialize()
{
  ShutdownUpdateTask();

  {
    ezLock<ezMutex> ml(m_CuratorMutex);

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


const ezAssetInfo* ezAssetCurator::FindAssetInfo(const char* szRelativePath) const
{
  ezStringBuilder sPath = szRelativePath;

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    return GetAssetInfo(ezConversionUtils::ConvertStringToUuid(sPath));
  }

  sPath.MakeCleanPath();

  if (sPath.IsAbsolutePath())
  {
    ezString sPath2 = sPath;
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath2))
      return nullptr;

    sPath = sPath2;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->m_sRelativePath.IsEqual_NoCase(sPath))
      return it.Value();
  }

  return nullptr;
}

void ezAssetCurator::CheckFileSystem()
{
  ezProgressRange range("Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  ezLock<ezMutex> ml(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  BuildFileExtensionSet(m_ValidAssetExtensions);

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(dd.m_sRelativePath);

    range.BeginNextStep(dd.m_sRelativePath);

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
}

ezString ezAssetCurator::FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const
{
  ezStringBuilder sAssetPath(szAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sDataDir(ezApplicationFileSystemConfig::GetProjectDirectory(), "/", dd.m_sRelativePath);

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  EZ_REPORT_FAILURE("Could not find data directory for asset '%s", szAbsoluteAssetPath);
  return ezApplicationFileSystemConfig::GetProjectDirectory();
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

  ezFileWriter file;
  if (file.Open(sFinalPath).Failed())
  {
    ezLog::Error("Failed to open asset lookup table file ('%s')", sFinalPath.GetData());
    return EZ_FAILURE;
  }

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

  return EZ_SUCCESS;
}

void ezAssetCurator::TransformAllAssets(const char* szPlatform /* = nullptr*/)
{
  ezProgressRange range("Transforming Assets", 1 + m_KnownAssets.GetCount(), true);

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (range.WasCanceled())
      break;

    range.BeginNextStep(ezPathUtils::GetFileNameAndExtension(it.Value()->m_sRelativePath).GetData());

    // Find the descriptor for the asset.
    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (ezDocumentManager::FindDocumentTypeFromPath(it.Value()->m_sAbsolutePath, false, pTypeDesc).Failed())
    {
      ezLog::Error("The asset '%s' could not be queried for its ezDocumentTypeDescriptor, skipping transform!", it.Value()->m_sRelativePath.GetData());
      continue;
    }

    // Skip assets that cannot be auto-transformed.
    EZ_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('%s')", it.Value()->m_sRelativePath.GetData());
    auto assetFlags = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager)->GetAssetDocumentTypeFlags(pTypeDesc);
    if (assetFlags.IsAnySet(ezAssetDocumentFlags::DisableTransform | ezAssetDocumentFlags::OnlyTransformManually))
      continue;

    ezUInt64 uiHash = 0;
    if (IsAssetUpToDate(it.Key(), szPlatform, pTypeDesc, uiHash))
      continue;

    if (uiHash == 0)
    {
      ezLog::Error("Computing the hash for asset '%s' or any dependency failed", it.Value()->m_sAbsolutePath.GetData());
      continue;
    }

    ezDocument* pDoc = ezQtEditorApp::GetSingleton()->OpenDocumentImmediate(it.Value()->m_sAbsolutePath, false, false);

    if (pDoc == nullptr)
    {
      ezLog::Error("Could not open asset document '%s'", it.Value()->m_sRelativePath.GetData());
      continue;
    }


    ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
    auto ret = pAsset->TransformAsset(szPlatform);

    if (ret.m_Result.Failed())
    {
      ezLog::Error("%s (%s)", ret.m_sMessage.GetData(), pDoc->GetDocumentPath());
    }

    if (!pDoc->HasWindowBeenRequested())
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  range.BeginNextStep("Writing Lookup Tables");

  ezAssetCurator::GetSingleton()->WriteAssetTables(szPlatform);
}

ezResult ezAssetCurator::WriteAssetTables(const char* szPlatform /* = nullptr*/)
{
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");

  ezResult res = EZ_SUCCESS;

  ezStringBuilder s;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    s.Set(ezApplicationFileSystemConfig::GetProjectDirectory(), "/", dd.m_sRelativePath);

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

void ezAssetCurator::MainThreadTick()
{
  ezLock<ezMutex> ml(m_CuratorMutex);

  static bool bReentry = false;
  if (bReentry)
    return;

  bReentry = true;

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

  bReentry = false;
}

void ezAssetCurator::UpdateAssetLastAccessTime(const ezUuid& assetGuid)
{
  ezAssetInfo* pAssetInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return;

  pAssetInfo->m_LastAccess = ezTime::Now();
}

const ezAssetInfo* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid) const
{
  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;

  return nullptr;
}

void ezAssetCurator::ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReader& stream)
{
  ezAbstractObjectGraph graph;
  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");
  rttiConverter.ApplyPropertiesToObject(pHeaderNode, pInfo->GetDynamicRTTI(), pInfo);
}

ezResult ezAssetCurator::EnsureAssetInfoUpdated(const ezUuid& assetGuid)
{
  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return EZ_FAILURE;

  return EnsureAssetInfoUpdated(pInfo->m_sAbsolutePath);
}

ezResult ezAssetCurator::EnsureAssetInfoUpdated(const char* szAbsFilePath)
{
  ezFileStats fs;
  if (ezOSFile::GetFileStats(szAbsFilePath, fs).Failed())
    return EZ_FAILURE;

  EZ_LOCK(m_CuratorMutex);

  if (m_ReferencedFiles[szAbsFilePath].m_Timestamp.IsEqual(fs.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
    return EZ_SUCCESS;

  ezAssetInfo assetInfo;
  FileStatus status;

  auto& RefFile = m_ReferencedFiles[szAbsFilePath];

  ezResult res = UpdateAssetInfo(szAbsFilePath, status, assetInfo, &fs);

  if (res.Succeeded())
  {
    if (RefFile.m_AssetGuid.IsValid() && RefFile.m_AssetGuid != status.m_AssetGuid)
    {
      m_KnownAssets[RefFile.m_AssetGuid]->m_ExistanceState = ezAssetInfo::ExistanceState::FileRemoved;

      auto& newAsset = m_KnownAssets[status.m_AssetGuid];
      if (newAsset == nullptr)
        newAsset = EZ_DEFAULT_NEW(ezAssetInfo);

      *newAsset = assetInfo;
    }
  }
  else
  {
    /// \todo Hide asset
  }

  RefFile = status;
  return res;
}

ezResult ezAssetCurator::UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetInfo& assetInfo, const ezFileStats* pFileStat)
{
  // try to read the asset JSON file
  ezFileReader file;
  if (file.Open(szAbsFilePath) == EZ_FAILURE)
  {
    stat.m_uiHash = 0;
    stat.m_AssetGuid = ezUuid();
    stat.m_Status = FileStatus::Status::FileLocked;

    ezLog::Error("Failed to open asset file '%s'", szAbsFilePath);
    return EZ_FAILURE;
  }


  // Update time stamp
  {
    ezFileStats fs;

    if (pFileStat == nullptr)
    {
      if (ezOSFile::GetFileStats(szAbsFilePath, fs).Failed())
        return EZ_FAILURE;

      pFileStat = &fs;
    }

    stat.m_Timestamp = pFileStat->m_LastModificationTime;
  }

  // update the paths
  {
    const ezStringBuilder sDataDir = GetSingleton()->FindDataDirectoryForAsset(szAbsFilePath);
    ezStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir);

    assetInfo.m_sRelativePath = sRelPath;
    assetInfo.m_sAbsolutePath = szAbsFilePath;
  }

  // if the file was previously tagged as "deleted", it is now "new" again (to ensure the proper events are sent)
  if (assetInfo.m_ExistanceState == ezAssetInfo::ExistanceState::FileRemoved)
  {
    assetInfo.m_ExistanceState = ezAssetInfo::ExistanceState::FileAdded;
  }

  // figure out which manager should handle this asset type
  {
    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (assetInfo.m_pManager == nullptr)
    {
      if (ezDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, false, pTypeDesc).Failed())
      {
        EZ_REPORT_FAILURE("Invalid asset setup");
      }

      assetInfo.m_pManager = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager);
    }
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(assetInfo.m_sAbsolutePath);

  ezMemoryStreamWriter MemWriter(&storage);

  // compute the hash for the asset JSON file
  stat.m_uiHash = ezAssetCurator::HashFile(file, &MemWriter);
  file.Close();

  // and finally actually read the asset JSON file (header only) and store the information in the ezAssetDocumentInfo member
  {
    ReadAssetDocumentInfo(&assetInfo.m_Info, MemReader);

    // here we get the GUID out of the JSON document
    // this links the 'file' to the 'asset'
    stat.m_AssetGuid = assetInfo.m_Info.m_DocumentID;
  }

  return EZ_SUCCESS;
}


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

const ezHashTable<ezUuid, ezAssetInfo*>& ezAssetCurator::GetKnownAssets() const
{
  return m_KnownAssets;
}

void ezAssetCurator::DocumentManagerEventHandler(const ezDocumentManager::Event& r)
{
  // Invoke new scan
}

