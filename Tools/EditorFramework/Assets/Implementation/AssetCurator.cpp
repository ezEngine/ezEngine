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
#include <ToolsFoundation/Serialization/DocumentJSONReader.h>
#include <ToolsFoundation/Serialization/SerializedTypeAccessorObject.h>
#include <ToolsFoundation/Object/SerializedDocumentObject.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

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
  ezString sExt;

  do
  {
    sPath = iterator.GetStats().m_sFileName.GetFileExtension();
    sPath.ToLower();
    sExt = sPath;

    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sFileName);
    sPath.MakeCleanPath();

    auto& RefFile = m_ReferencedFiles[sPath];
    RefFile.m_Status = FileStatus::Status::Valid;

    if (!RefFile.m_Timestamp.IsEqual(iterator.GetStats().m_LastModificationTime, ezTimestamp::CompareMode::Identical))
    {
      RefFile.m_Timestamp = iterator.GetStats().m_LastModificationTime;
      RefFile.m_uiHash = 0;
    }

    if (!validExtensions.Contains(sExt))
      continue;

    if (RefFile.m_AssetGuid.IsValid() && RefFile.m_uiHash != 0)
      continue;

    // Find Asset Info
    AssetInfo* pAssetInfo = nullptr;
    ezUuid oldGuid = RefFile.m_AssetGuid;
    bool bNew = false;
    if (RefFile.m_AssetGuid.IsValid())
    {
      EZ_VERIFY(m_KnownAssets.TryGetValue(RefFile.m_AssetGuid, pAssetInfo), "guid set in file-status but no asset is actually known under that guid");
    }
    else
    {
      bNew = true;
      pAssetInfo = EZ_DEFAULT_NEW(AssetInfo);
    }

    // Update
    UpdateAssetInfo(sPath, RefFile, *pAssetInfo); // ignore return value

    if (bNew)
    {
      EZ_ASSERT_DEV(pAssetInfo->m_Info.m_DocumentID.IsValid(), "Asset header read for '%s', but its GUID is invalid! Corrupted document?", sPath.GetData());
      m_KnownAssets[pAssetInfo->m_Info.m_DocumentID] = pAssetInfo;
    }
    else
    {
      if (oldGuid != RefFile.m_AssetGuid)
      {
        m_KnownAssets.Remove(oldGuid);
        m_KnownAssets.Insert(RefFile.m_AssetGuid, pAssetInfo);
      }
    }
  }
  while (iterator.Next().Succeeded());
}

void ezAssetCurator::SetAllAssetStatusUnknown()
{
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = FileStatus::Status::Unknown;
  }
}

void ezAssetCurator::RemoveStaleFileInfos()
{
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_Status == FileStatus::Status::Unknown)
    {
      if (it.Value().m_AssetGuid.IsValid())
      {
        AssetInfo* pCache = m_KnownAssets[it.Value().m_AssetGuid];

        if (it.Key() == pCache->m_sPath)
        {
          m_KnownAssets.Remove(pCache->m_Info.m_DocumentID);
          EZ_DEFAULT_DELETE(pCache);
        }
      }
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

void ezAssetCurator::CheckFileSystem()
{
  {
    ezLock<ezMutex> ml(m_HashingMutex);
    m_FileHashingQueue.Clear();
  }

  if (m_pHashingTask)
  {
    ezTaskSystem::WaitForTask((ezTask*)m_pHashingTask);

    ezLock<ezMutex> ml(m_HashingMutex);
    EZ_ASSERT_DEV(m_pHashingTask->IsTaskFinished(), "task should not have started again");
    EZ_DEFAULT_DELETE(m_pHashingTask);
  }

  ezLock<ezMutex> ml(m_HashingMutex);

  ezSet<ezString> validExtensions;
  BuildFileExtensionSet(validExtensions);
  SetAllAssetStatusUnknown();

  for (auto dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(dd.m_sRelativePath);

    IterateDataDirectory(sTemp, validExtensions);
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
    sTemp = it.Value()->m_sPath;

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
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    ezDocumentBase* pDoc = ezEditorApp::GetInstance()->OpenDocumentImmediate(it.Value()->m_sPath, false);

    if (pDoc == nullptr)
    {
      ezLog::Error("Could not open asset document '%s'", it.Value()->m_sPath.GetData());
      continue;
    }

    EZ_ASSERT_DEV(pDoc->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocument>(), "Asset document does not derive from correct base class ('%s')", it.Value()->m_sPath.GetData());

    ezAssetDocument* pAsset = static_cast<ezAssetDocument*>(pDoc);
    pAsset->TransformAsset();

    if (!pDoc->HasWindowBeenRequested())
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }
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

  return res;
}

void ezAssetCurator::MainThreadTick()
{
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

const ezAssetCurator::AssetInfo* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid) const
{
  AssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;

  return nullptr;
}

void ezAssetCurator::ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReaderBase& stream)
{
  ezDocumentJSONReader reader(&stream);

  if (reader.OpenGroup("Header"))
  {
    ezUuid objectGuid;
    ezStringBuilder sType;
    ezUuid parentGuid;
    while (reader.PeekNextObject(objectGuid, sType, parentGuid))
    {
      if (sType == pInfo->GetDynamicRTTI()->GetTypeName())
      {
        ezReflectedTypeHandle hType = ezReflectedTypeManager::GetTypeHandleByName(pInfo->GetDynamicRTTI()->GetTypeName());
        EZ_ASSERT_DEV(!hType.IsInvalidated(), "Need to register ezDocumentInfo at the ezReflectedTypeManager first!");

        ezReflectedTypeDirectAccessor acc(pInfo);
        ezSerializedTypeAccessorObjectReader objectReader(&acc);
        reader.ReadObject(objectReader);
      }
    }
  }
}

ezResult ezAssetCurator::UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetCurator::AssetInfo& assetInfo)
{
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
    if (ezOSFile::GetFileStats(file.GetFilePathAbsolute(), fs).Failed())
      return EZ_FAILURE;

    stat.m_Timestamp = fs.m_LastModificationTime;
  }

  assetInfo.m_sPath = szAbsFilePath;
  if (assetInfo.m_State == AssetInfo::State::ToBeDeleted)
  {
    assetInfo.m_State = AssetInfo::State::New;
  }

  ezDocumentManagerBase* pDocMan = assetInfo.m_pManager;
  if (assetInfo.m_pManager == nullptr && ezDocumentManagerBase::FindDocumentTypeFromPath(szAbsFilePath, false, pDocMan).Failed())
  {
    EZ_REPORT_FAILURE("Invalid asset setup");
  }
  assetInfo.m_pManager = static_cast<ezAssetDocumentManager*>(pDocMan);

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  ezMemoryStreamWriter MemWriter(&storage);

  stat.m_uiHash = ezAssetCurator::HashFile(file, &MemWriter);
  file.Close();

  ReadAssetDocumentInfo(&assetInfo.m_Info, MemReader);
  stat.m_AssetGuid = assetInfo.m_Info.m_DocumentID;
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

