#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

ezResult ezAssetTable::WriteAssetTable()
{
  EZ_PROFILE_SCOPE("WriteAssetTable");

  ezStringBuilder sTemp;
  ezString sResourcePath;

  {
    for (auto& man : ezAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
        continue;

      ezAssetDocumentManager* pManager = static_cast<ezAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(m_sDataDir, m_pProfile, ezMakeDelegate(&ezAssetTable::AddManagerResource, this));
    }
  }

  if (m_bReset)
  {
    m_GuidToPath.Clear();
    ezAssetCurator::ezLockedSubAssetTable allSubAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it = allSubAssetsLocked->GetIterator(); it.IsValid(); ++it)
    {
      sTemp = it.Value().m_pAssetInfo->m_Path.GetAbsolutePath();

      // ignore all assets that are not located in this data directory
      if (!sTemp.IsPathBelowFolder(m_sDataDir))
        continue;

      Update(it.Value());
    }
    m_bReset = false;
  }

  // We don't write anything on a background process as the main editor process will have already written any dirty tables before sending an RPC request. We still want to engine process to reload any potential changes though and be able to check which resources to reload so the tables are kept up to date in memory.
  if (ezQtEditorApp::GetSingleton()->IsBackgroundMode())
    return EZ_SUCCESS;

  ezDeferredFileWriter file;
  file.SetOutput(m_sTargetFile);

  auto Write = [](const ezString& sGuid, const ezString& sPath, ezDeferredFileWriter& ref_file)
  {
    ref_file.WriteBytes(sGuid.GetData(), sGuid.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes(";", 1).IgnoreResult();
    ref_file.WriteBytes(sPath.GetData(), sPath.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes("\n", 1).IgnoreResult();
  };

  for (auto it = m_GuidToManagerResource.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value().m_sPath, file);
  }

  for (auto it = m_GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value(), file);
  }

  if (file.Close().Failed())
  {
    ezLog::Error("Failed to open asset lookup table file '{0}'", m_sTargetFile);
    return EZ_FAILURE;
  }

  m_bDirty = false;
  return EZ_SUCCESS;
}

void ezAssetTable::Remove(const ezSubAsset& subAsset)
{
  ezStringBuilder sTemp;
  ezConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);
  m_GuidToPath.Remove(sTemp);
  m_bDirty = true;
}

void ezAssetTable::Update(const ezSubAsset& subAsset)
{
  ezStringBuilder sTemp;
  ezAssetDocumentManager* pManager = subAsset.m_pAssetInfo->GetManager();
  ezString sEntry = pManager->GetAssetTableEntry(&subAsset, m_sDataDir, m_pProfile);

  // It is valid to write no asset table entry, if no redirection is required. This is used by decal assets for instance.
  if (!sEntry.IsEmpty())
  {
    ezConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);

    m_GuidToPath[sTemp] = sEntry;
  }
  m_bDirty = true;
}

void ezAssetTable::AddManagerResource(ezStringView sGuid, ezStringView sPath, ezStringView sType)
{
  m_GuidToManagerResource[sGuid] = ManagerResource{sPath, sType};
}

ezAssetTableWriter::ezAssetTableWriter(const ezApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
  m_DataDirToAssetTables.SetCount(m_FileSystemConfig.m_DataDirs.GetCount());

  ezStringBuilder sDataDirPath;

  m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
  for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
  {
    if (ezFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      ezLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
      m_DataDirRoots.PushBack({});
    }
    else
    {
      m_DataDirRoots.PushBack(sDataDirPath);
    }
  }

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezAssetTableWriter::AssetCuratorEvents, this));
}

ezAssetTableWriter::~ezAssetTableWriter()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetTableWriter::AssetCuratorEvents, this));
}

void ezAssetTableWriter::MainThreadTick()
{
  // We must flush any pending table changes before triggering resource reloads.
  // If no resource reload is scheduled, we can just wait for the timer to run out to flush the changes.
  //
  if (m_bTablesDirty && (ezTime::Now() > m_NextTableFlush || m_bNeedToReloadResources))
  {
    m_bTablesDirty = false;
    if (WriteAssetTables(ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    {
      ezLog::Error("Failed to write asset tables");
    }
  }

  if (m_bNeedToReloadResources)
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
    EZ_LOCK(m_AssetTableMutex);

    bool bReloadManagerResources = false;
    const ezPlatformProfile* pCurrentProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile();
    for (const ReloadResource& reload : m_ReloadResources)
    {
      if (ezAssetTable* pTable = GetAssetTable(reload.m_uiDataDirIndex, pCurrentProfile))
      {
        if (pTable->m_GuidToPath.Contains(reload.m_sResource))
        {
          ezReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = reload.m_sResource;
          msg2.m_sResourceType = reload.m_sType;
          ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else if (ezPathUtils::IsAbsolutePath(reload.m_sResource))
        {
          if (reload.m_uiDataDirIndex >= m_DataDirRoots.GetCount())
            continue;

          ezStringBuilder sTempPath = reload.m_sResource;
          if (sTempPath.MakeRelativeTo(m_DataDirRoots[reload.m_uiDataDirIndex]).Failed())
            continue;

          ezReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = sTempPath;
          msg2.m_sResourceType = reload.m_sType;
          ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else
        {
          // If an asset is not represented by a resource in the table we assume it is represented by a manager resource.
          // Currently we don't know how these relate, e.g. we don't know all "Decal" assets are represented by the "{ ProjectDecalAtlas }" resource. Therefore, we just reload all manager resources.
          bReloadManagerResources = true;
        }
      }
    }
    m_ReloadResources.Clear();

    if (bReloadManagerResources)
    {
      for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        ezAssetTable* pTable = GetAssetTable(i, pCurrentProfile);
        for (auto it : pTable->m_GuidToManagerResource)
        {
          ezReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = it.Key();
          msg2.m_sResourceType = it.Value().m_sType;
          ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
      }
    }

    // This forces the deletion of cached render data.
    ezSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadResources";
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    m_bNeedToReloadResources = false;
  }
}

void ezAssetTableWriter::NeedsReloadResource(const ezUuid& assetGuid)
{
  ezAssetCurator::ezLockedSubAsset asset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (asset.isValid())
  {
    EZ_LOCK(m_AssetTableMutex);
    m_bNeedToReloadResources = true;
    ezString sDocType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetString();
    ezStringBuilder sGuid;
    ezConversionUtils::ToString(assetGuid, sGuid);
    const ezUInt32 uiDataDirIndex = FindDataDir(*asset);

    if (asset->m_bMainAsset)
    {
      const ezPlatformProfile* pProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile();
      const ezAssetDocumentManager* pManager = asset->m_pAssetInfo->GetManager();
      const ezAssetDocumentTypeDescriptor* pDocTypeDesc = asset->m_pAssetInfo->m_pDocumentTypeDescriptor;
      const ezSet<ezString>& outputs = asset->m_pAssetInfo->m_Info->m_Outputs;
      for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
      {
        // Additional outputs are not written to the asset table, so we assume they are referenced by a relative path in the runtime. We store an absolute path here though so that we can detect additional outputs inside the MainThreadTick function where we flush the resource reloads.
        const ezString sTargetFile = pManager->GetAbsoluteOutputFileName(pDocTypeDesc, asset->m_pAssetInfo->m_Path.GetAbsolutePath(), it.Key(), pProfile);
        const ezStringView sDocumentType = pManager->GetOutputDocumentType(pDocTypeDesc, it.Key(), pProfile);
        m_ReloadResources.PushBack({uiDataDirIndex, sTargetFile, sDocumentType});
      }
    }
    m_ReloadResources.PushBack({uiDataDirIndex, sGuid, sDocType});
  }
}

ezResult ezAssetTableWriter::WriteAssetTables(const ezPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  EZ_LOG_BLOCK("ezAssetCurator::WriteAssetTables");
  EZ_ASSERT_DEV(pAssetProfile != nullptr, "WriteAssetTables: pAssetProfile must be set.");

  ezResult res = EZ_SUCCESS;
  bool bAnyChanged = false;
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
    EZ_LOCK(m_AssetTableMutex);

    ezStringBuilder sd;

    for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      ezAssetTable* table = GetAssetTable(i, pAssetProfile);
      if (!table)
      {
        ezLog::Error("WriteAssetTables: The data dir '{}' with path '{}' could not be resolved", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        res = EZ_FAILURE;
        continue;
      }

      bAnyChanged |= (table->m_bReset || table->m_bDirty);
      if (!bForce && !table->m_bDirty && !table->m_bReset)
        continue;

      if (table->WriteAssetTable().Failed())
        res = EZ_FAILURE;
    }
  }

  if (bAnyChanged && pAssetProfile == ezAssetCurator::GetSingleton()->GetActiveAssetProfile())
  {
    ezSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload = pAssetProfile->GetConfigName();
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  m_NextTableFlush = ezTime::Now() + ezTime::MakeFromSeconds(1.5);
  return res;
}

void ezAssetTableWriter::AssetCuratorEvents(const ezAssetCuratorEvent& e)
{
  EZ_LOCK(m_AssetTableMutex);

  const ezPlatformProfile* pProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile();
  switch (e.m_Type)
  {
    // #TODO Are asset table entries static or do they change with the asset?
    /*case ezAssetCuratorEvent::Type::AssetUpdated:
      if (e.m_pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::TransformState::Unknown)
        return;
      [[fallthrough]];*/
    case ezAssetCuratorEvent::Type::AssetAdded:
    case ezAssetCuratorEvent::Type::AssetMoved:
    {
      ezUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (ezAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Update(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case ezAssetCuratorEvent::Type::AssetRemoved:
    {
      ezUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (ezAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Remove(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case ezAssetCuratorEvent::Type::AssetListReset:
      for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        for (auto it : m_DataDirToAssetTables[i])
        {
          it.Value()->m_bReset = true;
        }
      }
      m_bTablesDirty = true;
      break;
    case ezAssetCuratorEvent::Type::ActivePlatformChanged:
      if (WriteAssetTables(pProfile, false).Failed())
      {
        ezLog::Error("Failed to write asset tables");
      }
      break;
    default:
      break;
  }
}

ezAssetTable* ezAssetTableWriter::GetAssetTable(ezUInt32 uiDataDirIndex, const ezPlatformProfile* pAssetProfile)
{
  auto it = m_DataDirToAssetTables[uiDataDirIndex].Find(pAssetProfile);
  if (!it.IsValid())
  {
    if (m_DataDirRoots[uiDataDirIndex].IsEmpty())
      return nullptr;

    ezUniquePtr<ezAssetTable> table = EZ_DEFAULT_NEW(ezAssetTable);
    table->m_pProfile = pAssetProfile;
    table->m_sDataDir = m_DataDirRoots[uiDataDirIndex];

    ezStringBuilder sFinalPath(m_DataDirRoots[uiDataDirIndex], "/AssetCache/", pAssetProfile->GetConfigName(), ".ezAidlt");
    sFinalPath.MakeCleanPath();
    table->m_sTargetFile = sFinalPath;

    it = m_DataDirToAssetTables[uiDataDirIndex].Insert(pAssetProfile, std::move(table));
  }
  return it.Value().Borrow();
}

ezUInt32 ezAssetTableWriter::FindDataDir(const ezSubAsset& asset)
{
  return asset.m_pAssetInfo->m_Path.GetDataDirIndex();
}
