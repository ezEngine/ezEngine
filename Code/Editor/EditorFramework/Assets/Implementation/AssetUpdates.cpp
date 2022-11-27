#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

ezAssetInfo::TransformState ezAssetCurator::HashAsset(ezUInt64 uiSettingsHash, const ezHybridArray<ezString, 16>& assetTransformDependencies, const ezHybridArray<ezString, 16>& runtimeDependencies, ezSet<ezString>& missingDependencies, ezSet<ezString>& missingReferences, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  ezStringBuilder tmp;
  ezAssetInfo::TransformState state = ezAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDependencies)
    {
      ezString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingDependencies.Insert(sPath);
      }
    }

    for (const auto& dep : runtimeDependencies)
    {
      ezString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingReferences.Insert(sPath);
      }
    }
  }

  if (!missingReferences.IsEmpty())
  {
    out_ThumbHash = 0;
    state = ezAssetInfo::MissingReference;
  }
  if (!missingDependencies.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = ezAssetInfo::MissingDependency;
  }

  return state;
}

bool ezAssetCurator::AddAssetHash(ezString& sPath, bool bIsReference, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, bool bForce)
{
  if (sPath.IsEmpty())
    return true;

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    ezUInt64 assetHash = 0;
    ezUInt64 thumbHash = 0;
    ezAssetInfo::TransformState state = UpdateAssetTransformState(guid, assetHash, thumbHash, bForce);
    if (state == ezAssetInfo::Unknown || state == ezAssetInfo::MissingDependency || state == ezAssetInfo::MissingReference)
    {
      ezLog::Error("Failed to hash dependency asset '{0}'", sPath);
      return false;
    }

    // Thumbs hash is affected by both transform dependencies and references.
    out_ThumbHash += thumbHash;
    if (!bIsReference)
    {
      // References do not affect the asset hash.
      out_AssetHash += assetHash;
    }
    return true;
  }

  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
  {
    if (sPath.EndsWith(".color"))
    {
      // TODO: detect non-file assets and skip already in dependency gather function.
      return true;
    }
    ezLog::Error("Failed to make path absolute '{0}'", sPath);
    return false;
  }
  ezFileStats statDep;
  if (ezOSFile::GetFileStats(sPath, statDep).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '{0}'", sPath);
    return false;
  }

  ezFileStatus fileStatus;
  ezTimestamp previousModificationTime;
  {
    EZ_LOCK(m_CuratorMutex);
    fileStatus = m_ReferencedFiles[sPath];
    previousModificationTime = fileStatus.m_Timestamp;
  }

  // if the file has been modified, make sure to get updated data
  if (!fileStatus.m_Timestamp.Compare(statDep.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
  {
    CURATOR_PROFILE(sPath);
    ezFileReader file;
    if (file.Open(sPath).Failed())
    {
      ezLog::Error("Failed to open file '{0}'", sPath);
      return false;
    }
    fileStatus.m_Timestamp = statDep.m_LastModificationTime;
    fileStatus.m_uiHash = ezAssetCurator::HashFile(file, nullptr);
    fileStatus.m_Status = ezFileStatus::Status::Valid;
  }

  {
    EZ_LOCK(m_CuratorMutex);
    ezFileStatus& refFile = m_ReferencedFiles[sPath];
    // Only update the status if the file status has not been changed between the locks or we might write stale data to it.
    if (refFile.m_Timestamp.Compare(previousModificationTime, ezTimestamp::CompareMode::Identical))
    {
      refFile = fileStatus;
    }
  }

  // Thumbs hash is affected by both transform dependencies and references.
  out_ThumbHash += fileStatus.m_uiHash;
  if (!bIsReference)
  {
    // References do not affect the asset hash.
    out_AssetHash += fileStatus.m_uiHash;
  }
  return true;
}

ezResult ezAssetCurator::EnsureAssetInfoUpdated(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);
  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return EZ_FAILURE;

  // It is not safe here to pass pInfo->m_sAbsolutePath into EnsureAssetInfoUpdated
  // as the function is meant to change the very instance we are passing in.
  ezStringBuilder sAbsPath = pInfo->m_sAbsolutePath;
  return EnsureAssetInfoUpdated(sAbsPath);
}

static ezResult PatchAssetGuid(const char* szAbsFilePath, ezUuid oldGuid, ezUuid newGuid)
{
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, true, pTypeDesc).Failed())
    return EZ_FAILURE;

  if (ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szAbsFilePath))
  {
    pTypeDesc->m_pManager->CloseDocument(pDocument);
  }

  ezUInt32 uiTries = 0;
  while (pTypeDesc->m_pManager->CloneDocument(szAbsFilePath, szAbsFilePath, newGuid).Failed())
  {
    if (uiTries >= 5)
      return EZ_FAILURE;

    ezThreadUtils::Sleep(ezTime::Milliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  return EZ_SUCCESS;
}

ezResult ezAssetCurator::EnsureAssetInfoUpdated(const char* szAbsFilePath)
{
  CURATOR_PROFILE(szAbsFilePath);
  ezFileStats fs;
  {
    CURATOR_PROFILE("GetFileStats");
    if (ezOSFile::GetFileStats(szAbsFilePath, fs).Failed())
      return EZ_FAILURE;
  }
  {
    // If the file stat matches our stored timestamp, we are still up to date.
    EZ_LOCK(m_CuratorMutex);
    if (m_ReferencedFiles[szAbsFilePath].m_Timestamp.Compare(fs.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
      return EZ_SUCCESS;
  }

  // Read document info outside the lock
  ezFileStatus fileStatus;
  ezUniquePtr<ezAssetInfo> pNewAssetInfo;
  EZ_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(szAbsFilePath, fileStatus, pNewAssetInfo));
  EZ_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");

  EZ_LOCK(m_CuratorMutex);
  ezFileStatus& RefFile = m_ReferencedFiles[szAbsFilePath];
  ezUuid oldGuid = RefFile.m_AssetGuid;
  // if it already has a valid GUID, an ezAssetInfo object must exist
  bool bNew = !RefFile.m_AssetGuid.IsValid(); // Under this current location the asset is not known.
  EZ_VERIFY(bNew == !m_KnownAssets.Contains(RefFile.m_AssetGuid), "guid set in file-status but no asset is actually known under that guid");

  RefFile = fileStatus;
  ezAssetInfo* pOldAssetInfo = nullptr;
  if (bNew)
  {
    // now the GUID must be valid
    EZ_ASSERT_DEV(pNewAssetInfo->m_Info->m_DocumentID.IsValid(), "Asset header read for '{0}', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    EZ_ASSERT_DEV(RefFile.m_AssetGuid == pNewAssetInfo->m_Info->m_DocumentID, "UpdateAssetInfo broke the GUID!");

    // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
    m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pOldAssetInfo);
    if (pOldAssetInfo != nullptr)
    {
      if (pNewAssetInfo->m_sAbsolutePath == pOldAssetInfo->m_sAbsolutePath)
      {
        // As it is a new asset, this should actually never be the case.
        UntrackDependencies(pOldAssetInfo);
        pOldAssetInfo->Update(pNewAssetInfo);
        TrackDependencies(pOldAssetInfo);
        UpdateAssetTransformState(RefFile.m_AssetGuid, ezAssetInfo::TransformState::Unknown);
        SetAssetExistanceState(*pOldAssetInfo, ezAssetExistanceState::FileModified);
        UpdateSubAssets(*pOldAssetInfo);
        RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
        return EZ_SUCCESS;
      }
      else
      {
        ezFileStats fsOldLocation;
        if (ezOSFile::GetFileStats(pOldAssetInfo->m_sAbsolutePath, fsOldLocation).Failed())
        {
          // Asset moved, remove old file and asset info.
          m_ReferencedFiles.Remove(pOldAssetInfo->m_sAbsolutePath);
          UntrackDependencies(pOldAssetInfo);
          pOldAssetInfo->Update(pNewAssetInfo);
          TrackDependencies(pOldAssetInfo);
          UpdateAssetTransformState(RefFile.m_AssetGuid, ezAssetInfo::TransformState::Unknown);
          SetAssetExistanceState(*pOldAssetInfo,
            ezAssetExistanceState::FileModified); // asset was only moved, prevent added event (could have been modified though)
          UpdateSubAssets(*pOldAssetInfo);
          RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
          return EZ_SUCCESS;
        }
        else
        {
          // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
          // That means we currently always adjust the GUID of the second, third, etc. file that we look at
          // even if we might know that changing another file makes more sense
          // This works well for when the editor is running and someone copies a file.

          ezLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_sAbsolutePath, pOldAssetInfo->m_sAbsolutePath);

          const ezUuid mod = ezUuid::StableUuidForString(szAbsFilePath);
          ezUuid newGuid = pNewAssetInfo->m_Info->m_DocumentID;
          newGuid.CombineWithSeed(mod);

          if (PatchAssetGuid(szAbsFilePath, pNewAssetInfo->m_Info->m_DocumentID, newGuid).Failed())
          {
            ezLog::Error("Failed to adjust GUID of asset: '{0}'", szAbsFilePath);
            m_ReferencedFiles.Remove(szAbsFilePath);
            return EZ_FAILURE;
          }

          ezLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", szAbsFilePath);

          // now let's try that again
          m_ReferencedFiles.Remove(szAbsFilePath);
          return EnsureAssetInfoUpdated(szAbsFilePath);
        }
      }
    }

    // and we can store the new ezAssetInfo data under that GUID
    pOldAssetInfo = pNewAssetInfo.Release();
    m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
    TrackDependencies(pOldAssetInfo);
    UpdateAssetTransformState(pOldAssetInfo->m_Info->m_DocumentID, ezAssetInfo::TransformState::Unknown);
    UpdateSubAssets(*pOldAssetInfo);
  }
  else
  {
    // Guid changed, different asset found, mark old as deleted and add new one.
    if (oldGuid != RefFile.m_AssetGuid)
    {
      SetAssetExistanceState(*m_KnownAssets[oldGuid], ezAssetExistanceState::FileRemoved);
      RemoveAssetTransformState(oldGuid);

      if (RefFile.m_AssetGuid.IsValid())
      {
        pOldAssetInfo = pNewAssetInfo.Release();
        m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
        TrackDependencies(pOldAssetInfo);
        // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
        UpdateSubAssets(*pOldAssetInfo);
      }
    }
    else
    {
      // Update asset info
      pOldAssetInfo = m_KnownAssets[RefFile.m_AssetGuid];
      UntrackDependencies(pOldAssetInfo);
      pOldAssetInfo->Update(pNewAssetInfo);
      TrackDependencies(pOldAssetInfo);
      SetAssetExistanceState(*pOldAssetInfo, ezAssetExistanceState::FileModified);
      UpdateSubAssets(*pOldAssetInfo);
    }
  }

  InvalidateAssetTransformState(RefFile.m_AssetGuid);
  return EZ_SUCCESS;
}

void ezAssetCurator::TrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, true);

  const ezString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  UpdateUnresolvedTrackedFiles(m_InverseDependency, m_UnresolvedDependencies);
  UpdateUnresolvedTrackedFiles(m_InverseReferences, m_UnresolvedReferences);
}

void ezAssetCurator::UntrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, false);

  const ezString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

void ezAssetCurator::UpdateTrackedFiles(const ezUuid& assetGuid, const ezSet<ezString>& files, ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid>>& unresolved, bool bAdd)
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
      it.Value().RemoveAndCopy(assetGuid);
    }
  }
}

void ezAssetCurator::UpdateUnresolvedTrackedFiles(ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid>>& unresolved)
{
  for (auto it = unresolved.GetIterator(); it.IsValid();)
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

ezResult ezAssetCurator::ReadAssetDocumentInfo(const char* szAbsFilePath, ezFileStatus& stat, ezUniquePtr<ezAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);

  ezFileStats fs;
  if (ezOSFile::GetFileStats(szAbsFilePath, fs).Failed())
    return EZ_FAILURE;
  stat.m_Timestamp = fs.m_LastModificationTime;
  stat.m_Status = ezFileStatus::Status::Valid;

  // try to read the asset file
  ezFileReader file;
  if (file.Open(szAbsFilePath) == EZ_FAILURE)
  {
    stat.m_Timestamp.Invalidate();
    stat.m_uiHash = 0;
    stat.m_Status = ezFileStatus::Status::FileLocked;

    ezLog::Error("Failed to open asset file '{0}'", szAbsFilePath);
    return EZ_FAILURE;
  }

  out_assetInfo = EZ_DEFAULT_NEW(ezAssetInfo);
  ezUniquePtr<ezAssetDocumentInfo> docInfo;
  auto itFile = m_CachedFiles.Find(szAbsFilePath);
  {
    EZ_LOCK(m_CachedAssetsMutex);
    auto itAsset = m_CachedAssets.Find(szAbsFilePath);
    if (itAsset.IsValid())
    {
      docInfo = std::move(itAsset.Value());
      m_CachedAssets.Remove(itAsset);
    }
  }

  // update the paths
  {
    ezStringBuilder sDataDir = GetSingleton()->FindDataDirectoryForAsset(szAbsFilePath);
    sDataDir.PathParentDirectory();

    ezStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir).IgnoreResult();

    out_assetInfo->m_sDataDirParentRelativePath = sRelPath;
    out_assetInfo->m_sDataDirRelativePath = ezStringView(out_assetInfo->m_sDataDirParentRelativePath.FindSubString("/") + 1);
    out_assetInfo->m_sAbsolutePath = szAbsFilePath;
  }

  // figure out which manager should handle this asset type
  {
    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (ezDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, false, pTypeDesc).Failed())
      {
        EZ_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(out_assetInfo->m_sAbsolutePath);

  ezMemoryStreamWriter MemWriter(&storage);

  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, ezTimestamp::CompareMode::Identical))
  {
    stat.m_uiHash = itFile.Value().m_uiHash;
  }
  else
  {
    // compute the hash for the asset file
    stat.m_uiHash = ezAssetCurator::HashFile(file, &MemWriter);
  }
  file.Close();

  // and finally actually read the asset file (header only) and store the information in the ezAssetDocumentInfo member
  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, ezTimestamp::CompareMode::Identical))
  {
    out_assetInfo->m_Info = std::move(docInfo);
    stat.m_AssetGuid = out_assetInfo->m_Info->m_DocumentID;
  }
  else
  {
    ezStatus ret = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, MemReader);
    if (ret.Failed())
    {
      ezLog::Error("Failed to read asset document info for asset file '{0}'", szAbsFilePath);
      return EZ_FAILURE;
    }
    EZ_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");

    // here we get the GUID out of the document
    // this links the 'file' to the 'asset'
    stat.m_AssetGuid = out_assetInfo->m_Info->m_DocumentID;
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::UpdateSubAssets(ezAssetInfo& assetInfo)
{
  CURATOR_PROFILE("UpdateSubAssets");
  if (assetInfo.m_ExistanceState == ezAssetExistanceState::FileRemoved)
  {
    return;
  }

  if (assetInfo.m_ExistanceState == ezAssetExistanceState::FileAdded)
  {
    auto& mainSub = m_KnownSubAssets[assetInfo.m_Info->m_DocumentID];
    mainSub.m_bMainAsset = true;
    mainSub.m_ExistanceState = ezAssetExistanceState::FileAdded;
    mainSub.m_pAssetInfo = &assetInfo;
    mainSub.m_Data.m_Guid = assetInfo.m_Info->m_DocumentID;
    mainSub.m_Data.m_sSubAssetsDocumentTypeName = assetInfo.m_Info->m_sAssetsDocumentTypeName;
  }

  {
    ezHybridArray<ezSubAssetData, 4> subAssets;
    {
      CURATOR_PROFILE("FillOutSubAssetList");
      assetInfo.GetManager()->FillOutSubAssetList(*assetInfo.m_Info.Borrow(), subAssets);
    }

    for (const ezUuid& sub : assetInfo.m_SubAssets)
    {
      m_KnownSubAssets[sub].m_ExistanceState = ezAssetExistanceState::FileRemoved;
      m_SubAssetChanged.Insert(sub);
    }

    for (const ezSubAssetData& data : subAssets)
    {
      const bool bExisted = m_KnownSubAssets.Find(data.m_Guid).IsValid();
      EZ_ASSERT_DEV(bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid), "Implementation error: m_KnownSubAssets and assetInfo.m_SubAssets are out of sync.");

      ezSubAsset sub;
      sub.m_bMainAsset = false;
      sub.m_ExistanceState = bExisted ? ezAssetExistanceState::FileModified : ezAssetExistanceState::FileAdded;
      sub.m_pAssetInfo = &assetInfo;
      sub.m_Data = data;
      m_KnownSubAssets.Insert(data.m_Guid, sub);

      if (!bExisted)
      {
        assetInfo.m_SubAssets.Insert(sub.m_Data.m_Guid);
        m_SubAssetChanged.Insert(sub.m_Data.m_Guid);
      }
    }

    for (auto it = assetInfo.m_SubAssets.GetIterator(); it.IsValid();)
    {
      if (m_KnownSubAssets[it.Key()].m_ExistanceState == ezAssetExistanceState::FileRemoved)
      {
        it = assetInfo.m_SubAssets.Remove(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

ezUInt64 ezAssetCurator::HashFile(ezStreamReader& InputStream, ezStreamWriter* pPassThroughStream)
{
  ezHashStreamWriter64 hsw;

  CURATOR_PROFILE("HashFile");
  ezUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const ezUInt64 uiRead = InputStream.ReadBytes(cachedBytes, EZ_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).IgnoreResult();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).IgnoreResult();
  }

  return hsw.GetHashValue();
}

void ezAssetCurator::RemoveAssetTransformState(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);

  for (int i = 0; i < ezAssetInfo::TransformState::COUNT; i++)
  {
    m_TransformState[i].Remove(assetGuid);
  }
  m_TransformStateStale.Remove(assetGuid);
}


void ezAssetCurator::InvalidateAssetTransformState(const ezUuid& assetGuid)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    // We do not set pAssetInfo->m_TransformState because that is user facing and
    // as after updating the state it might just be the same as before we instead add
    // it to the queue here to prevent flickering in the GUI.
    m_TransformStateStale.Insert(assetGuid);
    // Increasing m_LastStateUpdate will ensure that asset hash/state computations
    // that are in flight will not be written back to the asset.
    pAssetInfo->m_LastStateUpdate++;
    pAssetInfo->m_AssetHash = 0;
    pAssetInfo->m_ThumbHash = 0;
    auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
    if (it.IsValid())
    {
      for (const ezUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
    if (it2.IsValid())
    {
      for (const ezUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }
  }
}

void ezAssetCurator::UpdateAssetTransformState(const ezUuid& assetGuid, ezAssetInfo::TransformState state)
{
  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    m_TransformStateStale.Remove(assetGuid);
    for (int i = 0; i < ezAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Remove(assetGuid);
    }
    m_TransformState[state].Insert(assetGuid);

    const bool bStateChanged = pAssetInfo->m_TransformState != state;

    if (bStateChanged)
    {
      pAssetInfo->m_TransformState = state;
      m_SubAssetChanged.Insert(assetGuid);
      for (const auto& key : pAssetInfo->m_SubAssets)
      {
        m_SubAssetChanged.Insert(key);
      }
    }

    switch (state)
    {
      case ezAssetInfo::TransformState::TransformError:
      {
        // Transform errors are unexpected and invalidate any previously computed
        // state of assets depending on this one.
        auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
        if (it.IsValid())
        {
          for (const ezUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
        if (it2.IsValid())
        {
          for (const ezUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        break;
      }

      case ezAssetInfo::TransformState::Unknown:
      {
        InvalidateAssetTransformState(assetGuid);
        break;
      }

      case ezAssetInfo::TransformState::UpToDate:
      {
        if (bStateChanged)
        {
          ezString sThumbPath = static_cast<ezAssetDocumentManager*>(pAssetInfo->GetManager())->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);
          ezQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
        }
        break;
      }

      default:
        break;
    }
  }
}

void ezAssetCurator::UpdateAssetTransformLog(const ezUuid& assetGuid, ezDynamicArray<ezLogEntry>& logEntries)
{
  ezAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    pAssetInfo->m_LogEntries.Clear();
    pAssetInfo->m_LogEntries.Swap(logEntries);
  }
}


void ezAssetCurator::SetAssetExistanceState(ezAssetInfo& assetInfo, ezAssetExistanceState::Enum state)
{
  EZ_ASSERT_DEBUG(m_CuratorMutex.IsLocked(), "");

  // Only the main thread tick function is allowed to change from FileAdded to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  if (assetInfo.m_ExistanceState != ezAssetExistanceState::FileAdded || state != ezAssetExistanceState::FileModified)
    assetInfo.m_ExistanceState = state;

  for (ezUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (existanceState != ezAssetExistanceState::FileAdded || state != ezAssetExistanceState::FileModified)
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (existanceState != ezAssetExistanceState::FileAdded || state != ezAssetExistanceState::FileModified)
  {
    existanceState = state;
    m_SubAssetChanged.Insert(assetInfo.m_Info->m_DocumentID);
  }
}


////////////////////////////////////////////////////////////////////////
// ezUpdateTask
////////////////////////////////////////////////////////////////////////

ezUpdateTask::ezUpdateTask(ezOnTaskFinishedCallback onTaskFinished)
{
  ConfigureTask("ezUpdateTask", ezTaskNesting::Never, onTaskFinished);
}

ezUpdateTask::~ezUpdateTask() = default;

void ezUpdateTask::Execute()
{
  ezUuid assetGuid;
  {
    EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);
    if (!ezAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
      return;
  }

  const ezDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  ezUInt64 uiAssetHash = 0;
  ezUInt64 uiThumbHash = 0;

  // Do not log update errors done on the background thread. Only if done explicitly on the main thread or the GUI will not be responsive
  // if the user deleted some base asset and everything starts complaining about it.
  ezLogEntryDelegate logger([&](ezLogEntry& entry) -> void {}, ezLogMsgType::All);
  ezLogSystemScope logScope(&logger);

  ezAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
