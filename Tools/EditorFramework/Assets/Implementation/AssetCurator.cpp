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
#include <GuiFoundation/UIServices/ProgressBar.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>

ezAssetCurator* ezAssetCurator::s_pInstance = nullptr;

ezAssetCurator::ezAssetCurator()
{
  m_bActive = false;
  s_pInstance = this;
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
}

ezAssetCurator::~ezAssetCurator()
{
  Deinitialize();

  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
  s_pInstance = nullptr;
}

void ezAssetCurator::BuildFileExtensionSet(ezSet<ezString>& AllExtensions)
{
  ezStringBuilder sTemp;
  AllExtensions.Clear();

  for (auto pMan : ezDocumentManagerBase::GetAllDocumentManagers())
  {
    if (pMan->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
    {
      ezHybridArray<ezDocumentTypeDescriptor, 4> DocumentTypes;
      pMan->GetSupportedDocumentTypes(DocumentTypes);

      for (const auto& DocType : DocumentTypes)
      {
        for (const auto& ext : DocType.m_sFileExtensions)
        {
          sTemp = ext;
          sTemp.ToLower();

          AllExtensions.Insert(sTemp);
        }
      }
    }
  }
}

void ezAssetCurator::NotifyOfPotentialAsset(const char* szAbsolutePath)
{
  HandleSingleFile(szAbsolutePath);

  /// \todo Move this into a regularly running thread or so
  MainThreadTick();
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath)
{
  ezFileStats Stats;
  if (ezOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
    return;

  // make sure the set exists, but don't update it
  // this is only updated in CheckFileSystem
  if (m_ValidAssetExtensions.IsEmpty())
    BuildFileExtensionSet(m_ValidAssetExtensions);

  HandleSingleFile(sAbsolutePath, m_ValidAssetExtensions, Stats);
}

void ezAssetCurator::HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat)
{
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
  AssetInfo* pAssetInfo = nullptr;
  ezUuid oldGuid = RefFile.m_AssetGuid;
  bool bNew = false;

  // if it already has a valid GUID, an AssetInfo object must exist
  if (RefFile.m_AssetGuid.IsValid())
  {
    EZ_VERIFY(m_KnownAssets.TryGetValue(RefFile.m_AssetGuid, pAssetInfo), "guid set in file-status but no asset is actually known under that guid");
  }
  else
  {
    // otherwise create a new AssetInfo object

    bNew = true;
    pAssetInfo = EZ_DEFAULT_NEW(AssetInfo);
  }

  // this will update m_AssetGuid by reading it from the asset JSON file
  UpdateAssetInfo(sAbsolutePath, RefFile, *pAssetInfo, &FileStat); // ignore return value

  if (bNew)
  {
    // now the GUID must be valid
    EZ_ASSERT_DEV(pAssetInfo->m_Info.m_DocumentID.IsValid(), "Asset header read for '%s', but its GUID is invalid! Corrupted document?", sAbsolutePath.GetData());
    EZ_ASSERT_DEV(RefFile.m_AssetGuid == pAssetInfo->m_Info.m_DocumentID, "UpdateAssetInfo broke the GUID!");
    // and we can store the new AssetInfo data under that GUID
    EZ_ASSERT_DEV(!m_KnownAssets.Contains(pAssetInfo->m_Info.m_DocumentID), "The assets '%s' and '%s' share the same GUID!", pAssetInfo->m_sAbsolutePath.GetData(), m_KnownAssets[pAssetInfo->m_Info.m_DocumentID]->m_sAbsolutePath.GetData());
    m_KnownAssets[pAssetInfo->m_Info.m_DocumentID] = pAssetInfo;
  }
  else
  {
    // in case the previous GUID is different to what UpdateAssetInfo read from file, get rid of the old GUID
    // and point the new one to the existing AssetInfo
    if (oldGuid != RefFile.m_AssetGuid)
    {
      m_KnownAssets.Remove(oldGuid);
      m_KnownAssets.Insert(RefFile.m_AssetGuid, pAssetInfo);
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

void ezAssetCurator::SetAllAssetStatusUnknown()
{
  // tags all known files as unknown, such that we can later remove files
  // that can not be found anymore

  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = FileStatus::Status::Unknown;
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
        // retrieve the AssetInfo data for this file
        AssetInfo* pCache = m_KnownAssets[it.Value().m_AssetGuid];

        // sanity check: if the AssetInfo really belongs to this file, remove it as well
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
  m_bActive = true;
  m_FileSystemConfig = cfg;
  m_sActivePlatform = "PC";

  CheckFileSystem();
}

void ezAssetCurator::Deinitialize()
{
  {
    ezLock<ezMutex> ml(m_HashingMutex);

    m_bActive = false;
    m_ReferencedFiles.Clear();
    m_FileHashingQueue.Clear();

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      EZ_DEFAULT_DELETE(it.Value());
    }
    m_KnownAssets.Clear();
    m_NeedsCheck.Clear();
    m_NeedsTransform.Clear();
    m_Done.Clear();
  }

  if (m_pHashingTask)
  {
    ezTaskSystem::WaitForTask((ezTask*)m_pHashingTask);

    ezLock<ezMutex> ml(m_HashingMutex);
    EZ_DEFAULT_DELETE(m_pHashingTask);
  }

  // Broadcast reset.
  {
    Event e;
    e.m_pInfo = nullptr;
    e.m_Type = Event::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
}


const ezAssetCurator::AssetInfo* ezAssetCurator::FindAssetInfo(const char* szRelativePath) const
{
  ezStringBuilder sPath = szRelativePath;
  sPath.MakeCleanPath();

  if (sPath.IsAbsolutePath())
  {
    ezString sPath2 = sPath;
    if (!ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(sPath2))
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
  // reset the queue of assets to be hashed
  {
    ezLock<ezMutex> ml(m_HashingMutex);
    m_FileHashingQueue.Clear();
  }

  QtProgressBar progress("Check Filesystem for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  if (m_pHashingTask)
  {
    ezTaskSystem::WaitForTask((ezTask*)m_pHashingTask);

    ezLock<ezMutex> ml(m_HashingMutex);
    EZ_ASSERT_DEV(m_pHashingTask->IsTaskFinished(), "task should not have started again");
    EZ_DEFAULT_DELETE(m_pHashingTask);
  }

  ezLock<ezMutex> ml(m_HashingMutex);

  SetAllAssetStatusUnknown();

  BuildFileExtensionSet(m_ValidAssetExtensions);

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(dd.m_sRelativePath);

    progress.WorkingOnNextItem(dd.m_sRelativePath);

    IterateDataDirectory(sTemp, m_ValidAssetExtensions);
  }

  RemoveStaleFileInfos();
  QueueFilesForHashing();

  // Broadcast reset.
  {
    Event e;
    e.m_pInfo = nullptr;
    e.m_Type = Event::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
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

ezResult ezAssetCurator::WriteAssetTable(const char* szDataDirectory)
{
  ezStringBuilder sDataDir = szDataDirectory;
  sDataDir.MakeCleanPath();

  ezStringBuilder sFinalPath(sDataDir, "/AssetCache/LookupTable.ezAsset");
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
    sResourcePath = it.Value()->m_pManager->GenerateRelativeResourceFileName(sDataDir, sTemp);

    sTemp.Format("%s;%s\n", ezConversionUtils::ToString(guid).GetData(), sResourcePath.GetData());

    file.WriteBytes(sTemp.GetData(), sTemp.GetElementCount());
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::TransformAllAssets()
{
  QtProgressBar progress("Transforming Assets", 1 + m_KnownAssets.GetCount(), true);

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (progress.WasProgressBarCanceled())
      break;

    progress.WorkingOnNextItem(ezPathUtils::GetFileNameAndExtension(it.Value()->m_sRelativePath).GetData());

    ezDocumentBase* pDoc = ezEditorApp::GetInstance()->OpenDocumentImmediate(it.Value()->m_sAbsolutePath, false);

    if (pDoc == nullptr)
    {
      ezLog::Error("Could not open asset document '%s'", it.Value()->m_sRelativePath.GetData());
      continue;
    }

    EZ_ASSERT_DEV(pDoc->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('%s')", it.Value()->m_sRelativePath.GetData());

    ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
    pAsset->TransformAsset();

    if (!pDoc->HasWindowBeenRequested())
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  progress.WorkingOnNextItem("Writing Lookup Tables");
  ezAssetCurator::GetInstance()->WriteAssetTables();
}

ezResult ezAssetCurator::WriteAssetTables()
{
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");

  ezResult res = EZ_SUCCESS;

  ezStringBuilder s;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    s.Set(ezApplicationFileSystemConfig::GetProjectDirectory(), "/", dd.m_sRelativePath);

    if (WriteAssetTable(s).Failed())
      res = EZ_FAILURE;
  }

  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadAssetLUT";
  ezEditorEngineProcessConnection::GetInstance()->SendMessage(&msg);

  msg.m_sWhatToDo = "ReloadResources";
  ezEditorEngineProcessConnection::GetInstance()->SendMessage(&msg);

  return res;
}

void ezAssetCurator::MainThreadTick()
{
  /// \todo This function is never called anywhere

  ezLock<ezMutex> ml(m_HashingMutex);

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid();)
  {
    if (it.Value()->m_State == ezAssetCurator::AssetInfo::State::New)
    {
      it.Value()->m_State = ezAssetCurator::AssetInfo::State::Default;

      // Broadcast reset.
      Event e;
      e.m_AssetGuid = it.Key();
      e.m_pInfo = it.Value();
      e.m_Type = Event::Type::AssetAdded;
      m_Events.Broadcast(e);

      ++it;
    }
    else if (it.Value()->m_State == ezAssetCurator::AssetInfo::State::ToBeDeleted)
    {
      // Broadcast reset.
      Event e;
      e.m_AssetGuid = it.Key();
      e.m_pInfo = it.Value();
      e.m_Type = Event::Type::AssetRemoved;
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
}

void ezAssetCurator::UpdateAssetLastAccessTime(const ezUuid& assetGuid)
{
  AssetInfo* pAssetInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return;

  pAssetInfo->m_LastAccess = ezTime::Now();
}

const ezAssetCurator::AssetInfo* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid) const
{
  AssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;

  return nullptr;
}

void ezAssetCurator::ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReaderBase& stream)
{
  ezAbstractObjectGraph graph;
  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");
  rttiConverter.ApplyPropertiesToObject(pHeaderNode, pInfo->GetDynamicRTTI(), pInfo);
}

ezResult ezAssetCurator::UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetCurator::AssetInfo& assetInfo, const ezFileStats* pFileStat)
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
    const ezStringBuilder sDataDir = GetInstance()->FindDataDirectoryForAsset(szAbsFilePath);
    ezStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir);

    assetInfo.m_sRelativePath = sRelPath;
    assetInfo.m_sAbsolutePath = szAbsFilePath;
  }

  // if the file was previously tagged as "deleted", it is now "new" again (to ensure the proper events are sent)
  if (assetInfo.m_State == AssetInfo::State::ToBeDeleted)
  {
    assetInfo.m_State = AssetInfo::State::New;
  }

  // figure out which manager should handle this asset type
  {
    ezDocumentManagerBase* pDocMan = assetInfo.m_pManager;
    if (assetInfo.m_pManager == nullptr && ezDocumentManagerBase::FindDocumentTypeFromPath(szAbsFilePath, false, pDocMan).Failed())
    {
      EZ_REPORT_FAILURE("Invalid asset setup");
    }
    assetInfo.m_pManager = static_cast<ezAssetDocumentManager*>(pDocMan);
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
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

const ezHashTable<ezUuid, ezAssetCurator::AssetInfo*>& ezAssetCurator::GetKnownAssets() const
{
  return m_KnownAssets;
}

void ezAssetCurator::DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r)
{
  // Invoke new scan
}

