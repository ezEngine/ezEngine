#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Algorithm/Hashing.h>
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
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezAssetCurator::ProjectEventHandler, this));
}

ezAssetCurator::~ezAssetCurator()
{
  ClearCache();

  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::DocumentManagerEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCurator::ProjectEventHandler, this));
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
  if (ezStringUtils::IsNullOrEmpty(szDataDir))
    return;

  ezFileSystemIterator iterator;
  if (iterator.StartSearch(szDataDir, true, false).Failed())
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

void ezAssetCurator::ClearCache()
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
    ezTaskSystem::WaitForTask((ezTask*) m_pHashingTask);

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
  ezLock<ezMutex> ml(m_HashingMutex);

  m_bActive = true;
  ezSet<ezString> validExtensions;
  BuildFileExtensionSet(validExtensions);
  SetAllAssetStatusUnknown();

  for (ezUInt32 dd = 0; dd < ezFileSystem::GetNumDataDirectories(); ++dd)
  {
    auto pDataDir = ezFileSystem::GetDataDirectory(dd);

    IterateDataDirectory(pDataDir->GetDataDirectoryPath(), validExtensions);
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

void ezAssetCurator::WriteAssetTables()
{
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

  if (assetInfo.m_pManager == nullptr && ezDocumentManagerBase::FindDocumentTypeFromPath(szAbsFilePath, false, assetInfo.m_pManager).Failed())
  {
    EZ_REPORT_FAILURE("Invalid asset setup");
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  ezMemoryStreamWriter MemWriter(&storage);

  stat.m_uiHash = ezAssetCurator::HashFile(file, &MemWriter);
  file.Close();

  ReadAssetDocumentInfo(&assetInfo.m_Info, MemReader);
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

void ezAssetCurator::ProjectEventHandler(const ezToolsProject::Event& e)
{
  if (e.m_Type == ezToolsProject::Event::Type::ProjectOpened)
  {
    CheckFileSystem();
  }
  if (e.m_Type == ezToolsProject::Event::Type::ProjectClosed)
  {
    ClearCache();
  }
}

