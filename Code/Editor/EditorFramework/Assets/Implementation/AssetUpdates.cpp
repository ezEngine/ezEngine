#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

ezAssetInfo::TransformState ezAssetCurator::HashAsset(ezUInt64 uiSettingsHash, const ezHybridArray<ezString, 16>& assetTransformDeps, const ezHybridArray<ezString, 16>& assetThumbnailDeps, ezSet<ezString>& missingTransformDeps, ezSet<ezString>& missingThumbnailDeps, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  ezStringBuilder tmp;
  ezAssetInfo::TransformState state = ezAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDeps)
    {
      ezString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingTransformDeps.Insert(sPath);
      }
    }

    for (const auto& dep : assetThumbnailDeps)
    {
      ezString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingThumbnailDeps.Insert(sPath);
      }
    }
  }

  if (!missingThumbnailDeps.IsEmpty())
  {
    out_ThumbHash = 0;
    state = ezAssetInfo::MissingThumbnailDependency;
  }
  if (!missingTransformDeps.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = ezAssetInfo::MissingTransformDependency;
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
    if (state == ezAssetInfo::Unknown || state == ezAssetInfo::MissingTransformDependency || state == ezAssetInfo::MissingThumbnailDependency || state == ezAssetInfo::CircularDependency)
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

  ezFileStatus fileStatus;
  ezResult res = ezFileSystemModel::GetSingleton()->HashFile(sPath, fileStatus);
  if (res.Failed())
  {
    return false;
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

static ezResult PatchAssetGuid(ezStringView sAbsFilePath, ezUuid oldGuid, ezUuid newGuid)
{
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sAbsFilePath, true, pTypeDesc).Failed())
    return EZ_FAILURE;

  if (ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sAbsFilePath))
  {
    pTypeDesc->m_pManager->CloseDocument(pDocument);
  }

  ezUInt32 uiTries = 0;
  ezStringBuilder sTemp = sAbsFilePath;
  while (pTypeDesc->m_pManager->CloneDocument(sTemp, sTemp, newGuid).Failed())
  {
    if (uiTries >= 5)
      return EZ_FAILURE;

    ezThreadUtils::Sleep(ezTime::Milliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  return EZ_SUCCESS;
}

ezResult ezAssetCurator::EnsureAssetInfoUpdated(ezStringView sAbsFilePath, const ezFileStatus& stat, bool bForce)
{
  CURATOR_PROFILE(sAbsFilePath);

  ezFileSystemModel* pFiles = ezFileSystemModel::GetSingleton();

  // Read document info outside the lock
  ezUniquePtr<ezAssetInfo> pNewAssetInfo;
  EZ_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(sAbsFilePath, stat, pNewAssetInfo));
  EZ_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");

  EZ_LOCK(m_CuratorMutex);
  const ezUuid oldGuid = stat.m_DocumentID;
  // if it already has a valid GUID, an ezAssetInfo object must exist
  const bool bNewAssetFile = !stat.m_DocumentID.IsValid(); // Under this current location the asset is not known.
  ezUuid newGuid = pNewAssetInfo->m_Info->m_DocumentID;

  ezAssetInfo* pCurrentAssetInfo = nullptr;
  // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
  m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pCurrentAssetInfo);
  EZ_VERIFY(bNewAssetFile == !pCurrentAssetInfo, "guid set in file-status but no asset is actually known under that guid");

  if (bNewAssetFile && pCurrentAssetInfo != nullptr)
  {
    ezFileStats fsOldLocation;
    if (!ezFileSystemModel::IsSameFile(pNewAssetInfo->m_sAbsolutePath, pCurrentAssetInfo->m_sAbsolutePath))
    {
      if (ezOSFile::GetFileStats(pCurrentAssetInfo->m_sAbsolutePath, fsOldLocation).Succeeded())
      {
        // DUPLICATED
        // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
        // That means we currently always adjust the GUID of the second, third, etc. file that we look at
        // even if we might know that changing another file makes more sense
        // This works well for when the editor is running and someone copies a file.

        ezLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_sAbsolutePath, pCurrentAssetInfo->m_sAbsolutePath);

        const ezUuid mod = ezUuid::StableUuidForString(sAbsFilePath);
        ezUuid replacementGuid = pNewAssetInfo->m_Info->m_DocumentID;
        replacementGuid.CombineWithSeed(mod);

        // ReadAssetDocumentInfo already linked the file to the duplicate GUID. We remove the link again so that after patching the document we don't run into the wrong code path here.
        pFiles->UnlinkDocument(sAbsFilePath).IgnoreResult();
        if (PatchAssetGuid(sAbsFilePath, pNewAssetInfo->m_Info->m_DocumentID, replacementGuid).Failed())
        {
          ezLog::Error("Failed to adjust GUID of asset: '{0}'", sAbsFilePath);
          pFiles->NotifyOfChange(sAbsFilePath);
          return EZ_FAILURE;
        }

        ezLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", sAbsFilePath);

        // now let's try that again
        pFiles->NotifyOfChange(sAbsFilePath);
        return EZ_SUCCESS;
      }
      else
      {
        // MOVED
        // Notify old location to removed stale entry.
        pFiles->NotifyOfChange(pCurrentAssetInfo->m_sAbsolutePath);
      }
    }
  }

  // Guid changed, different asset found, mark old as deleted and add new one.
  if (!bNewAssetFile && oldGuid != pNewAssetInfo->m_Info->m_DocumentID)
  {
    // OVERWRITTEN
    SetAssetExistanceState(*m_KnownAssets[oldGuid], ezAssetExistanceState::FileRemoved);
    RemoveAssetTransformState(oldGuid);
  }

  bool bNewAsset = false;
  if (pCurrentAssetInfo)
  {
    UntrackDependencies(pCurrentAssetInfo);
    pCurrentAssetInfo->Update(pNewAssetInfo);
  }
  else
  {
    bNewAsset = true;
    pCurrentAssetInfo = pNewAssetInfo.Release();
    m_KnownAssets[newGuid] = pCurrentAssetInfo;
  }

  TrackDependencies(pCurrentAssetInfo);
  CheckForCircularDependencies(pCurrentAssetInfo).IgnoreResult();
  UpdateAssetTransformState(newGuid, ezAssetInfo::TransformState::Unknown);
  // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
  if (!bNewAsset)
    SetAssetExistanceState(*pCurrentAssetInfo, ezAssetExistanceState::FileModified);
  UpdateSubAssets(*pCurrentAssetInfo);

  InvalidateAssetTransformState(newGuid);

  return EZ_SUCCESS;
}

void ezAssetCurator::TrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, true);

  const ezString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  // Depending on the order of loading, dependencies might be unresolved until the dependency itself is loaded into the curator.
  // If pAssetInfo was previously an unresolved dependency, these two calls will update the inverse dep tables now that it can be resolved.
  UpdateUnresolvedTrackedFiles(m_InverseTransformDeps, m_UnresolvedTransformDeps);
  UpdateUnresolvedTrackedFiles(m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps);
}

void ezAssetCurator::UntrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, false);

  const ezString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

ezResult ezAssetCurator::CheckForCircularDependencies(ezAssetInfo* pAssetInfo)
{
  ezSet<ezUuid> inverseHull;
  GenerateInverseTransitiveHull(pAssetInfo, inverseHull, true, true);

  ezResult res = EZ_SUCCESS;
  for (const auto& sDep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    if (ezConversionUtils::IsStringUuid(sDep))
    {
      const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = EZ_FAILURE;
      }
    }
  }

  for (const auto& sDep : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    if (ezConversionUtils::IsStringUuid(sDep))
    {
      const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = EZ_FAILURE;
      }
    }
  }
  return res;
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

ezResult ezAssetCurator::ReadAssetDocumentInfo(ezStringView sAbsFilePath, const ezFileStatus& stat, ezUniquePtr<ezAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);
  ezFileSystemModel* pFiles = ezFileSystemModel::GetSingleton();

  out_assetInfo = EZ_DEFAULT_NEW(ezAssetInfo);

  // update the paths
  {
    ezStringBuilder sDataDir = FindDataDirectoryForAsset(sAbsFilePath);
    sDataDir.PathParentDirectory();

    ezStringBuilder sRelPath = sAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir).IgnoreResult();

    out_assetInfo->m_sDataDirParentRelativePath = sRelPath;
    out_assetInfo->m_sDataDirRelativePath = ezStringView(out_assetInfo->m_sDataDirParentRelativePath.FindSubString("/") + 1);
    out_assetInfo->m_sAbsolutePath = sAbsFilePath;
  }

  // figure out which manager should handle this asset type
  {
    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (ezDocumentManager::FindDocumentTypeFromPath(sAbsFilePath, false, pTypeDesc).Failed())
      {
        EZ_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  // Try cache first
  {
    ezFileStatus cacheStat;
    ezUniquePtr<ezAssetDocumentInfo> docInfo;
    {
      EZ_LOCK(m_CachedAssetsMutex);
      auto itFile = m_CachedFiles.Find(sAbsFilePath);
      auto itAsset = m_CachedAssets.Find(sAbsFilePath);
      if (itAsset.IsValid() && itFile.IsValid())
      {
        docInfo = std::move(itAsset.Value());
        cacheStat = itFile.Value();
        m_CachedAssets.Remove(itAsset);
        m_CachedFiles.Remove(itFile);
      }
    }

    if (docInfo && cacheStat.m_LastModified.Compare(stat.m_LastModified, ezTimestamp::CompareMode::Identical))
    {
      out_assetInfo->m_Info = std::move(docInfo);
      if (pFiles->LinkDocument(sAbsFilePath, out_assetInfo->m_Info->m_DocumentID).Failed())
      {
        // #TODO_ASSET
        ezLog::Error("Failed to link asset: {}", sAbsFilePath);
      }
      return EZ_SUCCESS;
    }
  }

  // try to read the asset file
  ezStatus infoStatus;
  ezResult res = pFiles->ReadDocument(sAbsFilePath, [&out_assetInfo, &infoStatus](const ezFileStatus& stat, ezStreamReader& ref_reader) -> ezUuid {
      infoStatus = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, ref_reader);
      // Here we return the GUID of the document. This links the 'file' to the 'asset'.
      // This is the same as later calling ezAssetFiles::LinkAsset
      return out_assetInfo->m_Info->m_DocumentID; });

  if (infoStatus.Failed())
  {
    ezLog::Error("Failed to read asset document info for asset file '{0}'", sAbsFilePath);
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");
  return res;
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

  ezSet<ezUuid> hull;
  {
    ezAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      GenerateInverseTransitiveHull(pAssetInfo, hull, true, true);
    }
  }

  for (const ezUuid& guid : hull)
  {
    ezAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pAssetInfo))
    {
      // We do not set pAssetInfo->m_TransformState because that is user facing and
      // as after updating the state it might just be the same as before we instead add
      // it to the queue here to prevent flickering in the GUI.
      m_TransformStateStale.Insert(guid);
      // Increasing m_LastStateUpdate will ensure that asset hash/state computations
      // that are in flight will not be written back to the asset.
      pAssetInfo->m_LastStateUpdate++;
      pAssetInfo->m_AssetHash = 0;
      pAssetInfo->m_ThumbHash = 0;
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
        auto it = m_InverseTransformDeps.Find(pAssetInfo->m_sAbsolutePath);
        if (it.IsValid())
        {
          for (const ezUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(pAssetInfo->m_sAbsolutePath);
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
  ezLogEntryDelegate logger([&](ezLogEntry& ref_entry) -> void {}, ezLogMsgType::All);
  ezLogSystemScope logScope(&logger);

  ezAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
