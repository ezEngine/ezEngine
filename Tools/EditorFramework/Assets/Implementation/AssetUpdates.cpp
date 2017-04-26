#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

////////////////////////////////////////////////////////////////////////
// ezAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

ezUInt64 ezAssetCurator::GetAssetHash(ezUuid assetGuid, bool bReferences)
{
  ezStringBuilder tmp;

  if (EnsureAssetInfoUpdated(assetGuid).Failed())
  {
    ezLog::Error("Asset with GUID {0} is unknown", ezConversionUtils::ToString(assetGuid, tmp));
    return 0;
  }

  EZ_LOCK(m_CuratorMutex);

  ezAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
  {
    ezLog::Error("Asset with GUID {0} is unknown", ezConversionUtils::ToString(assetGuid, tmp));
    return 0;
  }

  EZ_LOG_BLOCK("ezAssetCurator::GetAssetHash", pInfo->m_sAbsolutePath.GetData());

  ezFileStats stat;

  if (ezOSFile::GetFileStats(pInfo->m_sAbsolutePath, stat).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '{0}'", pInfo->m_sAbsolutePath);
    return 0;
  }

  if (pInfo->m_TransformState == ezAssetInfo::TransformError)
    return 0;

  if (bReferences)
    pInfo->m_MissingReferences.Clear();

  pInfo->m_MissingDependencies.Clear();

  // hash of the main asset file
  ezUInt64 uiHashResult = pInfo->m_Info.m_uiSettingsHash;

  // Iterate dependencies
  ezSet<ezString>& files = bReferences ? pInfo->m_Info.m_RuntimeDependencies : pInfo->m_Info.m_AssetTransformDependencies;

  for (const auto& dep : pInfo->m_Info.m_AssetTransformDependencies)
  {
    ezString sPath = dep;

    if (!AddAssetHash(sPath, bReferences, uiHashResult))
    {
      pInfo->m_MissingDependencies.Insert(sPath);
      return 0;
    }
  }

  if (bReferences)
  {
    for (const auto& dep : pInfo->m_Info.m_RuntimeDependencies)
    {
      ezString sPath = dep;
      // Already hashed in the dependency list.
      if (pInfo->m_Info.m_AssetTransformDependencies.Contains(sPath))
        continue;

      if (!AddAssetHash(sPath, bReferences, uiHashResult))
      {
        pInfo->m_MissingReferences.Insert(sPath);
        return 0;
      }
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
      ezLog::Error("Failed to hash dependency asset '{0}'", sPath.GetData());
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
    ezLog::Error("Failed to make path absolute '{0}'", sPath);
    return false;
  }
  ezFileStats statDep;
  if (ezOSFile::GetFileStats(sPath, statDep).Failed())
  {
    ezLog::Error("Failed to retrieve file stats '{0}'", sPath);
    return false;
  }

  auto& fileref = m_ReferencedFiles[sPath];

  // if the file has been modified, make sure to get updated data
  if (!fileref.m_Timestamp.Compare(statDep.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
  {
    ezFileReader file;
    if (file.Open(sPath).Failed())
    {
      ezLog::Error("Failed to open file '{0}'", sPath.GetData());
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

static ezResult PatchAssetGuid(const char* szAbsFilePath, ezUuid oldGuid, ezUuid newGuid)
{
  ezStringBuilder sContent;

  {
    ezFileReader file;
    ezUInt32 uiTries = 0;

    while (file.Open(szAbsFilePath).Failed())
    {
      if (uiTries >= 5)
        return EZ_FAILURE;

      ezThreadUtils::Sleep(ezTime::Milliseconds(50 * (uiTries + 1)));
      uiTries++;
    }

    sContent.ReadAll(file);
  }

  {
    // DDL
    ezUInt64 oldL, oldH;
    oldGuid.GetValues(oldL, oldH);

    ezUInt64 newL, newH;
    newGuid.GetValues(newL, newH);

    ezStringBuilder sOld;
    sOld.Format("{0},{1}", oldL, oldH);

    ezStringBuilder sNew;
    sNew.Format("{0},{1}", newL, newH);

    sContent.ReplaceAll(sOld, sNew);
  }

  {
    ezFileWriter file;

    ezUInt32 uiTries = 0;

    while (file.Open(szAbsFilePath).Failed())
    {
      if (uiTries >= 5)
        return EZ_FAILURE;

      ezThreadUtils::Sleep(ezTime::Milliseconds(50 * (uiTries + 1)));
      uiTries++;
    }

    return file.WriteBytes(sContent.GetData(), sContent.GetElementCount());
  }

  return EZ_SUCCESS;
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
  EZ_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(szAbsFilePath, RefFile, assetInfo, &fs));

  ezAssetInfo* pAssetInfo = nullptr;
  if (bNew)
  {
    // now the GUID must be valid
    EZ_ASSERT_DEV(assetInfo.m_Info.m_DocumentID.IsValid(), "Asset header read for '{0}', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    EZ_ASSERT_DEV(RefFile.m_AssetGuid == assetInfo.m_Info.m_DocumentID, "UpdateAssetInfo broke the GUID!");

    // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
    m_KnownAssets.TryGetValue(assetInfo.m_Info.m_DocumentID, pAssetInfo);
    if (pAssetInfo != nullptr)
    {
      if (assetInfo.m_sAbsolutePath == pAssetInfo->m_sAbsolutePath)
      {
        // As it is a new asset, this should actually never be the case.
        UntrackDependencies(pAssetInfo);
        pAssetInfo->Update(assetInfo);
        TrackDependencies(pAssetInfo);
        UpdateAssetTransformState(RefFile.m_AssetGuid, ezAssetInfo::TransformState::Unknown);
        SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileModified);
        UpdateSubAssets(*pAssetInfo);
        RefFile.m_AssetGuid = pAssetInfo->m_Info.m_DocumentID;
        return EZ_SUCCESS;
      }
      else
      {
        ezFileStats fsOldLocation;
        if (ezOSFile::GetFileStats(pAssetInfo->m_sAbsolutePath, fsOldLocation).Failed())
        {
          // Asset moved, remove old file and asset info.
          m_ReferencedFiles.Remove(pAssetInfo->m_sAbsolutePath);
          UntrackDependencies(pAssetInfo);
          // Copy old list of sub-assets so we don't lose it.
          assetInfo.m_SubAssets = pAssetInfo->m_SubAssets;
          EZ_DEFAULT_DELETE(pAssetInfo);
          pAssetInfo = nullptr;
          SetAssetExistanceState(assetInfo, ezAssetExistanceState::FileModified); // asset was only moved, prevent added event (could have been modified though)
        }
        else
        {
          // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
          // That means we currently always adjust the GUID of the second, third, etc. file that we look at
          // even if we might know that changing another file makes more sense
          // This works well for when the editor is running and someone copies a file.

          ezLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", assetInfo.m_sAbsolutePath.GetData(), pAssetInfo->m_sAbsolutePath.GetData());

          const ezUuid mod = ezUuid::StableUuidForString(szAbsFilePath);
          ezUuid newGuid = assetInfo.m_Info.m_DocumentID;
          newGuid.CombineWithSeed(mod);

          if (PatchAssetGuid(szAbsFilePath, assetInfo.m_Info.m_DocumentID, newGuid).Failed())
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
    pAssetInfo = EZ_DEFAULT_NEW(ezAssetInfo, assetInfo);
    m_KnownAssets[RefFile.m_AssetGuid] = pAssetInfo;
    TrackDependencies(pAssetInfo);
    UpdateAssetTransformState(pAssetInfo->m_Info.m_DocumentID, ezAssetInfo::TransformState::Unknown);
    UpdateSubAssets(*pAssetInfo);
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
        pAssetInfo = EZ_DEFAULT_NEW(ezAssetInfo, assetInfo);
        m_KnownAssets[RefFile.m_AssetGuid] = pAssetInfo;
        TrackDependencies(pAssetInfo);
        SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileAdded);
        UpdateSubAssets(*pAssetInfo);
      }
    }
    else
    {
      // Update asset info
      pAssetInfo = m_KnownAssets[RefFile.m_AssetGuid];
      UntrackDependencies(pAssetInfo);
      pAssetInfo->Update(assetInfo);
      TrackDependencies(pAssetInfo);
      SetAssetExistanceState(*pAssetInfo, ezAssetExistanceState::FileModified);
      UpdateSubAssets(*pAssetInfo);
    }
  }

  InvalidateAssetTransformState(RefFile.m_AssetGuid);
  return EZ_SUCCESS;
}

void ezAssetCurator::TrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, true);
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, true);

  const ezString sTargetFile = pAssetInfo->m_pManager->GetAbsoluteOutputFileName(pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info.m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info.m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile = pAssetInfo->m_pManager->GetAbsoluteOutputFileName(pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile);
    it.Value().PushBack(pAssetInfo->m_Info.m_DocumentID);
  }

  UpdateUnresolvedTrackedFiles(m_InverseDependency, m_UnresolvedDependencies);
  UpdateUnresolvedTrackedFiles(m_InverseReferences, m_UnresolvedReferences);
}

void ezAssetCurator::UntrackDependencies(ezAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, false);
  UpdateTrackedFiles(pAssetInfo->m_Info.m_DocumentID, pAssetInfo->m_Info.m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, false);

  const ezString sTargetFile = pAssetInfo->m_pManager->GetAbsoluteOutputFileName(pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().Remove(pAssetInfo->m_Info.m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info.m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const ezString sTargetFile = pAssetInfo->m_pManager->GetAbsoluteOutputFileName(pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile);
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

ezResult ezAssetCurator::ReadAssetDocumentInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetInfo& out_assetInfo, const ezFileStats* pFileStat)
{
  // try to read the asset file
  ezFileReader file;
  if (file.Open(szAbsFilePath) == EZ_FAILURE)
  {
    stat.m_Timestamp.Invalidate();
    stat.m_uiHash = 0;
    stat.m_Status = FileStatus::Status::FileLocked;

    ezLog::Error("Failed to open asset file '{0}'", szAbsFilePath);
    return EZ_FAILURE;
  }

  auto it = m_CachedEntries.Find(szAbsFilePath);
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
    stat.m_Status = FileStatus::Status::Valid;
  }

  // update the paths
  {
    const ezStringBuilder sDataDir = GetSingleton()->FindDataDirectoryForAsset(szAbsFilePath);
    ezStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir);

    out_assetInfo.m_sDataDirRelativePath = sRelPath;
    out_assetInfo.m_sAbsolutePath = szAbsFilePath;
  }

  // figure out which manager should handle this asset type
  {
    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo.m_pManager == nullptr)
    {
      if (ezDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, false, pTypeDesc).Failed())
      {
        EZ_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo.m_pManager = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager);
    }
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(out_assetInfo.m_sAbsolutePath);

  ezMemoryStreamWriter MemWriter(&storage);

  if (it.IsValid() && it.Value()->m_Timestamp.Compare(stat.m_Timestamp, ezTimestamp::CompareMode::Identical))
  {
    stat.m_uiHash = it.Value()->m_uiHash;
  }
  else
  {
    // compute the hash for the asset file
    stat.m_uiHash = ezAssetCurator::HashFile(file, &MemWriter);
  }
  file.Close();

  // and finally actually read the asset file (header only) and store the information in the ezAssetDocumentInfo member
  if (it.IsValid() && it.Value()->m_Timestamp.Compare(stat.m_Timestamp, ezTimestamp::CompareMode::Identical))
  {
    out_assetInfo.m_Info = it.Value()->m_Info;
    stat.m_AssetGuid = out_assetInfo.m_Info.m_DocumentID;
  }
  else
  {
    ezStatus ret = out_assetInfo.m_pManager->ReadAssetDocumentInfo(&out_assetInfo.m_Info, MemReader);
    if (ret.Failed())
    {
      ezLog::Error("Failed to read asset document info for asset file '{0}'", szAbsFilePath);
      return EZ_FAILURE;
    }

    // here we get the GUID out of the document
    // this links the 'file' to the 'asset'
    stat.m_AssetGuid = out_assetInfo.m_Info.m_DocumentID;
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::UpdateSubAssets(ezAssetInfo& assetInfo)
{
  if (assetInfo.m_ExistanceState == ezAssetExistanceState::FileRemoved)
  {
    auto itMain = m_KnownSubAssets.Find(assetInfo.m_Info.m_DocumentID);
    EZ_ASSERT_DEV(itMain.IsValid(), "Main SubAsset should have been added in the file FileAdded state change");

    itMain.Value().m_ExistanceState = ezAssetExistanceState::FileRemoved;

    for (const auto& sub : assetInfo.m_SubAssets)
    {
      auto itSub = m_KnownSubAssets.Find(sub);
      EZ_ASSERT_DEV(itMain.IsValid(), "SubAsset should have been added in the file FileAdded state change");

      itSub.Value().m_ExistanceState = ezAssetExistanceState::FileRemoved;
    }

    return;
  }

  if (assetInfo.m_ExistanceState == ezAssetExistanceState::FileAdded)
  {
    EZ_ASSERT_DEV(assetInfo.m_SubAssets.IsEmpty(), "Sub Asset list should be empty");

    auto& mainSub = m_KnownSubAssets[assetInfo.m_Info.m_DocumentID];
    mainSub.m_bMainAsset = true;
    mainSub.m_ExistanceState = ezAssetExistanceState::FileAdded;
    mainSub.m_pAssetInfo = &assetInfo;
    mainSub.m_Data.m_Guid = assetInfo.m_Info.m_DocumentID;
    mainSub.m_Data.m_sAssetTypeName.Assign(assetInfo.m_Info.m_sAssetTypeName.GetData());
  }

  if (assetInfo.m_ExistanceState == ezAssetExistanceState::FileModified)
  {
    auto& mainSub = m_KnownSubAssets[assetInfo.m_Info.m_DocumentID];
    mainSub.m_ExistanceState = ezAssetExistanceState::FileModified;
  }


  {
    ezHybridArray<ezSubAssetData, 4> subAssets;
    assetInfo.m_pManager->FillOutSubAssetList(assetInfo.m_Info, subAssets);

    for (const ezUuid& sub : assetInfo.m_SubAssets)
    {
      m_KnownSubAssets[sub].m_ExistanceState = ezAssetExistanceState::FileRemoved;
      m_SubAssetChanged.Insert(sub);
    }

    for (const ezSubAssetData& data : subAssets)
    {
      bool bExisted = false;
      auto& sub = m_KnownSubAssets.FindOrAdd(data.m_Guid, &bExisted).Value();
      EZ_ASSERT_DEV(bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid), "Implementation error: m_KnownSubAssets and assetInfo.m_SubAssets are out of sync.");
      sub.m_bMainAsset = false;
      sub.m_ExistanceState = bExisted ? ezAssetExistanceState::FileModified : ezAssetExistanceState::FileAdded;
      sub.m_pAssetInfo = &assetInfo;
      sub.m_Data = data;

      if (!bExisted)
      {
        assetInfo.m_SubAssets.Insert(sub.m_Data.m_Guid);
        m_SubAssetChanged.Insert(sub.m_Data.m_Guid);
      }
    }

    for (auto it = assetInfo.m_SubAssets.GetIterator(); it.IsValid(); )
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
    m_TransformStateStale.Insert(assetGuid);

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

    if (pAssetInfo->m_TransformState != state)
    {
      pAssetInfo->m_TransformState = state;
      m_SubAssetChanged.Insert(assetGuid);
      m_SubAssetChanged.Union(pAssetInfo->m_SubAssets);
    }

    switch (state)
    {
    case ezAssetInfo::TransformState::TransformError:
      {
        // Tansform errors are unexpected and invalidate any previously computed
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
      }
      break;
    case ezAssetInfo::TransformState::Unknown:
      {
        InvalidateAssetTransformState(assetGuid);
      }
      break;
    case ezAssetInfo::TransformState::UpToDate:
      {
        ezString sThumbPath = static_cast<ezAssetDocumentManager*>(pAssetInfo->m_pManager)->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);
        UpdateSubAssets(*pAssetInfo);
        if (pAssetInfo->m_TransformState != state)
        {
          ezQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
        }
      }
      break;
    default:
      break;
    }
  }
}

void ezAssetCurator::SetAssetExistanceState(ezAssetInfo& assetInfo, ezAssetExistanceState::Enum state)
{
  assetInfo.m_ExistanceState = state;
  m_SubAssetChanged.Insert(assetInfo.m_Info.m_DocumentID);
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
  {
    EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);
    if (!ezAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
      return;

    ezAssetCurator::GetSingleton()->UpdateAssetTransformState(assetGuid, ezAssetInfo::TransformState::Updating);
  }

  const ezDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  ezUInt64 uiAssetHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, nullptr, pTypeDescriptor, uiAssetHash, uiThumbHash);
}

