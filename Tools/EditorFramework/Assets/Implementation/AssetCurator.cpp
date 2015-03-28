#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Algorithm/Hashing.h>
#include <ToolsFoundation/Serialization/DocumentJSONReader.h>
#include <ToolsFoundation/Serialization/SerializedTypeAccessorObject.h>
#include <ToolsFoundation/Object/SerializedDocumentObject.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

ezAssetCurator* ezAssetCurator::s_pInstance = nullptr;

ezAssetCurator::ezAssetCurator()
{
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

    UpdateAssetInfo(sPath, RefFile);
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
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); )
  {
    if (it.Value().m_Status == FileStatus::Status::Unknown)
    {
      if (it.Value().m_AssetGuid.IsValid())
      {
        AssetInfoCache* pCache = m_KnownAssets[it.Value().m_AssetGuid];

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
  m_ReferencedFiles.Clear();

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    EZ_DEFAULT_DELETE(it.Value());
  }
  m_KnownAssets.Clear();
  m_NeedsCheck.Clear();
  m_NeedsTransform.Clear();
  m_Done.Clear();

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
  ezSet<ezString> validExtensions;
  BuildFileExtensionSet(validExtensions);
  SetAllAssetStatusUnknown();

  for (ezUInt32 dd = 0; dd < ezFileSystem::GetNumDataDirectories(); ++dd)
  {
    auto pDataDir = ezFileSystem::GetDataDirectory(dd);

    IterateDataDirectory(pDataDir->GetDataDirectoryPath(), validExtensions);
  }

  RemoveStaleFileInfos();

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

const ezAssetCurator::AssetInfoCache* ezAssetCurator::GetAssetInfo(const ezUuid& assetGuid) const
{
  AssetInfoCache* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;

  return nullptr;
}

ezAssetCurator::AssetInfoCache* ezAssetCurator::UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat)
{
  ezFileReader file;
  if (file.Open(szAbsFilePath) == EZ_FAILURE)
  {
    stat.m_uiHash = 0;
    stat.m_AssetGuid = ezUuid();
    stat.m_Status = FileStatus::Status::FileLocked;

    ezLog::Error("Failed to open asset file '%s'", szAbsFilePath);
    return nullptr;
  }

  AssetInfoCache* pAssetInfo = nullptr;

  bool bNew = false;
  if (stat.m_AssetGuid.IsValid())
  {
    EZ_VERIFY(m_KnownAssets.TryGetValue(stat.m_AssetGuid, pAssetInfo), "guid set in filestatus but no asset is actually known under that guid");
  }
  else
  {
    bNew = true;
    pAssetInfo = EZ_DEFAULT_NEW(AssetInfoCache);
  }

  pAssetInfo->m_sPath = szAbsFilePath;

  if (ezDocumentManagerBase::FindDocumentTypeFromPath(szAbsFilePath, false, pAssetInfo->m_pManager).Failed())
  {
    EZ_REPORT_FAILURE("Invalid asset setup");
  }

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader MemReader(&storage);
  ezMemoryStreamWriter MemWriter(&storage);


  {
    ezUInt8 uiCache[1024 * 10];
    stat.m_uiHash = 0;
    
    while (true)
    {
      ezUInt64 uiRead = file.ReadBytes(uiCache, EZ_ARRAY_SIZE(uiCache));

      if (uiRead == 0)
        break;

      stat.m_uiHash = ezHashing::MurmurHash64(uiCache, (size_t) uiRead, stat.m_uiHash);

      MemWriter.WriteBytes(uiCache, uiRead);
    }

    file.Close();
  }


  ezDocumentJSONReader reader(&MemReader);

  if (reader.OpenGroup("Header"))
  {
    ezUuid objectGuid;
    ezStringBuilder sType;
    ezUuid parentGuid;
    while (reader.PeekNextObject(objectGuid, sType, parentGuid))
    {
      if (sType == pAssetInfo->m_Info.GetDynamicRTTI()->GetTypeName())
      {
        ezReflectedTypeHandle hType = ezReflectedTypeManager::GetTypeHandleByName(pAssetInfo->m_Info.GetDynamicRTTI()->GetTypeName());
        EZ_ASSERT_DEV(!hType.IsInvalidated(), "Need to register ezDocumentInfo at the ezReflectedTypeManager first!");

        ezReflectedTypeDirectAccessor acc(&pAssetInfo->m_Info);
        ezSerializedTypeAccessorObjectReader objectReader(&acc);
        reader.ReadObject(objectReader);
      }
    }
  }

  if (bNew)
  {
    EZ_ASSERT_DEV(pAssetInfo->m_Info.m_DocumentID.IsValid(), "Asset header read for '%s', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    m_KnownAssets[pAssetInfo->m_Info.m_DocumentID] = pAssetInfo;
  }
 
  return pAssetInfo;
}

const ezHashTable<ezUuid, ezAssetCurator::AssetInfoCache*>& ezAssetCurator::GetKnownAssets() const
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

