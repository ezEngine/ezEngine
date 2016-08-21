#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/MemoryStream.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <IPC/ProcessCommunication.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

ezUInt64 ezAssetCurator::GetAssetHash(ezUuid assetGuid, bool bReferences)
{
  if (EnsureAssetInfoUpdated(assetGuid).Failed())
  {
    ezLog::Error("Asset with GUID %s is unknown", ezConversionUtils::ToString(assetGuid).GetData());
    return 0;
  }

  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
  {
    ezLog::Error("Asset with GUID %s is unknown", ezConversionUtils::ToString(assetGuid).GetData());
    return 0;
  }

  EZ_LOG_BLOCK("ezAssetCurator::GetAssetHash", pInfo->m_sAbsolutePath.GetData());

  ezFileStats stat;

  if (ezOSFile::GetFileStats(pInfo->m_sAbsolutePath, stat).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '%s'", pInfo->m_sAbsolutePath.GetData());
    return 0;
  }

  // hash of the main asset file
  ezUInt64 uiHashResult = pInfo->m_Info.m_uiSettingsHash;

  // Iterate dependencies
  ezSet<ezString>& files = bReferences ? pInfo->m_Info.m_FileReferences : pInfo->m_Info.m_FileDependencies;

  for (const auto& dep : pInfo->m_Info.m_FileDependencies)
  {
    ezString sPath = dep;

    if (!AddAssetHash(sPath, bReferences, uiHashResult))
      return 0;
  }

  if (bReferences)
  {
    for (const auto& dep : pInfo->m_Info.m_FileReferences)
    {
      ezString sPath = dep;
      // Already hashed in the dependency list.
      if (pInfo->m_Info.m_FileDependencies.Contains(sPath))
        continue;

      if (!AddAssetHash(sPath, bReferences, uiHashResult))
        return 0;
    }
  }

  if (!bReferences)
  {
    if (pInfo->m_LastAssetDependencyHash != uiHashResult)
    {
      pInfo->m_LastAssetDependencyHash = uiHashResult;
    }
  }
  return uiHashResult;
}

bool ezAssetCurator::AddAssetHash(ezString& sPath, bool bReferences, ezUInt64& uiHashResult)
{
  if (sPath.IsEmpty())
    return true;

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    ezUInt64 uiAssetHash = GetAssetHash(guid, bReferences);
    if (uiAssetHash == 0)
    {
      ezLog::Error("Failed to hash dependency asset '%s'", sPath.GetData());
      return false;
    }
    uiHashResult += uiAssetHash;
    return true;
  }

  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
  {
    if (sPath.EndsWith(".color"))
    {
      // TODO: detect non-file assets and skip already in dependency gather function.
      return true;
    }
    ezLog::Error("Failed to make path absolute '%s'", sPath.GetData());
    return 0;
  }
  ezFileStats statDep;
  if (ezOSFile::GetFileStats(sPath, statDep).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '%s'", sPath.GetData());
    return false;
  }

  auto& fileref = m_ReferencedFiles[sPath];

  // if the file has been modified, make sure to get updated data
  if (!fileref.m_Timestamp.Compare(statDep.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
  {
    ezFileReader file;
    if (file.Open(sPath).Failed())
    {
      ezLog::Error("Failed to open file '%s'", sPath.GetData());
      return false;
    }
    fileref.m_Timestamp = statDep.m_LastModificationTime;
    fileref.m_uiHash = ezAssetCurator::HashFile(file, nullptr);
    fileref.m_Status = ezAssetCurator::FileStatus::Status::Valid;
  }

  // combine all hashes
  uiHashResult += fileref.m_uiHash;
  return true;
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

  if (m_ReferencedFiles[szAbsFilePath].m_Timestamp.Compare(fs.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
    return EZ_SUCCESS;

  FileStatus& RefFile = m_ReferencedFiles[szAbsFilePath];
  ezAssetInfo assetInfo;
  ezUuid oldGuid = RefFile.m_AssetGuid;
  bool bNew = !RefFile.m_AssetGuid.IsValid(); // Under this current location the asset is not known.
  // if it already has a valid GUID, an ezAssetInfo object must exist
  EZ_VERIFY(bNew == !m_KnownAssets.Contains(RefFile.m_AssetGuid), "guid set in file-status but no asset is actually known under that guid");
  ezResult res = UpdateAssetInfo(szAbsFilePath, RefFile, assetInfo, &fs);

  if (res.Failed())
  {
    return res;
  }

  ezAssetInfo* pAssetInfo = nullptr;
  if (bNew)
  {
    // now the GUID must be valid
    EZ_ASSERT_DEV(assetInfo.m_Info.m_DocumentID.IsValid(), "Asset header read for '%s', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    EZ_ASSERT_DEV(RefFile.m_AssetGuid == assetInfo.m_Info.m_DocumentID, "UpdateAssetInfo broke the GUID!");

    // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
    m_KnownAssets.TryGetValue(assetInfo.m_Info.m_DocumentID, pAssetInfo);
    if (pAssetInfo != nullptr)
    {
      ezFileStats fsOldLocation;
      if (ezOSFile::GetFileStats(pAssetInfo->m_sAbsolutePath, fsOldLocation).Failed())
      {
        // Asset moved, remove old file and asset info.
        m_ReferencedFiles.Remove(pAssetInfo->m_sAbsolutePath);
        UntrackDependencies(pAssetInfo);
        EZ_DEFAULT_DELETE(pAssetInfo);
        pAssetInfo = nullptr;
      }
      else
      {
        EZ_REPORT_FAILURE("The assets '%s' and '%s' share the same GUID!", 
          assetInfo.m_sAbsolutePath.GetData(), 
          pAssetInfo->m_sAbsolutePath.GetData());
      }

    }
    // and we can store the new ezAssetInfo data under that GUID
    pAssetInfo = EZ_DEFAULT_NEW(ezAssetInfo, assetInfo);
    m_KnownAssets[RefFile.m_AssetGuid] = pAssetInfo;
    TrackDependencies(pAssetInfo);
    EZ_ASSERT_DEBUG(pAssetInfo->m_TransformState == ezAssetInfo::TransformState::Unknown, "State caching desynced!");
  }
  else
  {
    // Guid changed, different asset found, mark old as deleted and add new one.
    if (oldGuid != RefFile.m_AssetGuid)
    {
      m_KnownAssets[oldGuid]->m_ExistanceState = ezAssetInfo::ExistanceState::FileRemoved;
      m_TransformStateUnknown.Remove(oldGuid);
      m_TransformStateNeedsTransform.Remove(oldGuid);
      m_TransformStateNeedsThumbnail.Remove(oldGuid);

      if (RefFile.m_AssetGuid.IsValid())
      {
        pAssetInfo = EZ_DEFAULT_NEW(ezAssetInfo, assetInfo);
        m_KnownAssets[RefFile.m_AssetGuid] = pAssetInfo;
        TrackDependencies(pAssetInfo);
        
      }
    }
    else
    {
      // Update asset info
      pAssetInfo = m_KnownAssets[RefFile.m_AssetGuid];
      UntrackDependencies(pAssetInfo);
      *pAssetInfo = assetInfo;
      TrackDependencies(pAssetInfo);
    }
  }

  UpdateAssetTransformState(RefFile.m_AssetGuid, ezAssetInfo::TransformState::Unknown);

  return res;
}

void ezAssetCurator::TrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_FileDependencies, m_InverseDependency, m_UnresolvedDependencies, true);
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_FileReferences, m_InverseReferences, m_UnresolvedReferences, true);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pAssetInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(nullptr);
    const ezString sTargetFile = pAssetInfo->m_pManager->GetFinalOutputFileName(pTypeDesc, pAssetInfo->m_sAbsolutePath, sPlatform);
    auto it = m_InverseReferences.FindOrAdd(sTargetFile);
    it.Value().PushBack(pAssetInfo->m_Info.m_DocumentID);
  }

  UpdateUnresolvedTrackedFiles(m_InverseDependency, m_UnresolvedDependencies);
  UpdateUnresolvedTrackedFiles(m_InverseReferences, m_UnresolvedReferences);
}

void ezAssetCurator::UntrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_FileDependencies, m_InverseDependency, m_UnresolvedDependencies, false);
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_FileReferences, m_InverseReferences, m_UnresolvedReferences, false);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pAssetInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(nullptr);
    const ezString sTargetFile = pAssetInfo->m_pManager->GetFinalOutputFileName(pTypeDesc, pAssetInfo->m_sAbsolutePath, sPlatform);
    auto it = m_InverseReferences.FindOrAdd(sTargetFile);
    it.Value().Remove(pAssetInfo->m_Info.m_DocumentID);
  }
}

void ezAssetCurator::UpdateTrackedFiles(const ezUuid& assetGuid, const ezSet<ezString>& files, ezMap<ezString, ezHybridArray<ezUuid, 1> >& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid> >& unresolved, bool bAdd)
{
  for (const auto& dep : files)
  {
    ezString sPath = dep;

    if (sPath.IsEmpty())
      continue;

    if (ezConversionUtils::IsStringUuid(sPath))
    {
      const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
      const ezAssetInfo* pInfo = GetAssetInfo(guid);

      if (!bAdd)
      {
        unresolved.Remove(std::tuple<ezUuid, ezUuid>(assetGuid, guid));
        if (pInfo == nullptr)
          continue;
      }

      if (pInfo == nullptr && bAdd)
      {
        unresolved.Insert(std::tuple<ezUuid, ezUuid>(assetGuid, guid));
        continue;
      }

      sPath = pInfo->m_sAbsolutePath;
    }
    else
    {
      if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        continue;
      }
    }
    auto it = inverseTracker.FindOrAdd(sPath);
    if (bAdd)
    {
      it.Value().PushBack(assetGuid);
    }
    else
    {
      it.Value().Remove(assetGuid);
    }
  }
}

void ezAssetCurator::UpdateUnresolvedTrackedFiles(ezMap<ezString, ezHybridArray<ezUuid, 1> >& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid> >& unresolved)
{
  for (auto it = unresolved.GetIterator(); it.IsValid(); )
  {
    auto& t = *it;
    const ezUuid& assetGuid = std::get<0>(t);
    const ezUuid& depGuid = std::get<1>(t);
    if (const ezAssetInfo* pInfo = GetAssetInfo(depGuid))
    {
      ezString sPath = pInfo->m_sAbsolutePath;
      auto itTracker = inverseTracker.FindOrAdd(sPath);
      itTracker.Value().PushBack(assetGuid);
      it = unresolved.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

ezResult ezAssetCurator::UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetInfo& assetInfo, const ezFileStats* pFileStat)
{
  // try to read the asset JSON file
  ezFileReader file;
  if (file.Open(szAbsFilePath) == EZ_FAILURE)
  {
    stat.m_Timestamp.Invalidate();
    stat.m_uiHash = 0;
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

    assetInfo.m_sDataDirRelativePath = sRelPath;
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

void ezAssetCurator::ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReader& stream)
{
  ezAbstractObjectGraph graph;
  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");
  rttiConverter.ApplyPropertiesToObject(pHeaderNode, pInfo->GetDynamicRTTI(), pInfo);
}

ezUInt64 ezAssetCurator::HashFile(ezStreamReader& InputStream, ezStreamWriter* pPassThroughStream)
{
  ezUInt8 uiCache[1024 * 10];
  ezUInt64 uiHash = 0;

  while (true)
  {
    ezUInt64 uiRead = InputStream.ReadBytes(uiCache, EZ_ARRAY_SIZE(uiCache));

    if (uiRead == 0)
      break;

    uiHash = ezHashing::MurmurHash64(uiCache, (size_t)uiRead, uiHash);

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(uiCache, uiRead);
  }

  return uiHash;
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
      {
        m_TransformStateUnknown.Insert(assetGuid);
        m_TransformStateUpdating.Remove(assetGuid);
        m_TransformStateNeedsTransform.Remove(assetGuid);
        m_TransformStateNeedsThumbnail.Remove(assetGuid);

        auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
        if (it.IsValid())
        {
          for (const ezUuid& guid : it.Value())
          {
            UpdateAssetTransformState(guid, state);
          }
        }

        auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
        if (it2.IsValid())
        {
          for (const ezUuid& guid : it2.Value())
          {
            UpdateAssetTransformState(guid, state);
          }
        }
      }
      break;
    case ezAssetInfo::TransformState::Updating:
      m_TransformStateUnknown.Remove(assetGuid);
      m_TransformStateUpdating.Insert(assetGuid);
      m_TransformStateNeedsTransform.Remove(assetGuid);
      m_TransformStateNeedsThumbnail.Remove(assetGuid);
      break;
    case ezAssetInfo::TransformState::NeedsTransform:
      m_TransformStateUnknown.Remove(assetGuid);
      m_TransformStateUpdating.Remove(assetGuid);
      m_TransformStateNeedsTransform.Insert(assetGuid);
      m_TransformStateNeedsThumbnail.Remove(assetGuid);
      break;
    case ezAssetInfo::TransformState::NeedsThumbnail:
      m_TransformStateUnknown.Remove(assetGuid);
      m_TransformStateUpdating.Remove(assetGuid);
      m_TransformStateNeedsTransform.Remove(assetGuid);
      m_TransformStateNeedsThumbnail.Insert(assetGuid);
      break;
    case ezAssetInfo::TransformState::UpToDate:
      {
        m_TransformStateUnknown.Remove(assetGuid);
        m_TransformStateUpdating.Remove(assetGuid);
        m_TransformStateNeedsTransform.Remove(assetGuid);
        m_TransformStateNeedsThumbnail.Remove(assetGuid);
        ezString sThumbPath = static_cast<ezAssetDocumentManager*>(pAssetInfo->m_pManager)->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);
        ezQtImageCache::InvalidateCache(sThumbPath);
      }
      break;
    }
  }
}


////////////////////////////////////////////////////////////////////////
// ezUpdateTask
////////////////////////////////////////////////////////////////////////

ezUpdateTask::ezUpdateTask()
{
}

void ezUpdateTask::Execute()
{
  ezUuid assetGuid;
  if (!ezAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
    return;

  const ezDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  ezUInt64 uiAssetHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, nullptr, pTypeDescriptor, uiAssetHash, uiThumbHash);
}


////////////////////////////////////////////////////////////////////////
// ezProcessTask
////////////////////////////////////////////////////////////////////////

ezProcessTask::ezProcessTask()
  : m_bProcessShouldBeRunning(false), m_bProcessCrashed(false), m_bWaiting(false), m_bSuccess(true)
{
  m_pIPC = EZ_DEFAULT_NEW(ezProcessCommunication);
  m_pIPC->m_Events.AddEventHandler(ezMakeDelegate(&ezProcessTask::EventHandlerIPC, this));
}

ezProcessTask::~ezProcessTask()
{
  ShutdownProcess();
}

void ezProcessTask::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  if (const ezProcessAssetResponse* pMsg = ezDynamicCast<const ezProcessAssetResponse*>(e.m_pMessage))
  {
    m_bSuccess = pMsg->m_bSuccess;
    m_bWaiting = false;
  }
}

void ezProcessTask::StartProcess()
{
  const ezRTTI* pFirstAllowedMessageType = nullptr;
  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  QStringList args;
  args << "-appname";
  args << "ezEditorProcessor";
  args << "-project";
  args << ezToolsProject::GetSingleton()->GetProjectFile().GetData();

  if (m_pIPC->StartClientProcess("EditorProcessor.exe", args, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
}

void ezProcessTask::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  m_bProcessShouldBeRunning = false;
  m_pIPC->CloseConnection();
  EZ_DEFAULT_DELETE(m_pIPC);
}

void ezProcessTask::Execute()
{
  m_bSuccess = true;
  if (!ezAssetCurator::GetSingleton()->GetNextAssetToProcess(m_assetGuid, m_sAssetPath))
  {
    m_assetGuid = ezUuid();
    m_sAssetPath.Clear();
    return;
  }

  if (!m_bProcessShouldBeRunning)
  {
    StartProcess();
  }

  if (m_bProcessCrashed)
  {
    m_bSuccess = false;
    ezLog::Error("AssetProcessor crashed!");
    return;
  }

  ezLog::Info("Processing '%s'", m_sAssetPath.GetData());
  // Send and wait
  ezProcessAsset msg;
  msg.m_AssetGuid = m_assetGuid;
  msg.m_sAssetPath = m_sAssetPath;
  m_pIPC->SendMessage(&msg);
  m_bWaiting = true;

  while (m_bWaiting)
  {
    m_pIPC->ProcessMessages();
    ezThreadUtils::Sleep(10);
  }

  if (m_bSuccess)
  {
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(m_assetGuid);
    ezAssetCurator::GetSingleton()->m_bNeedToReloadResources = true;
  }
}
