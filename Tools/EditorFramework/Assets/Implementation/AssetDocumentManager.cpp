#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezBitflags<ezAssetDocumentFlags> ezAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::Default;
}

ezStatus ezAssetDocumentManager::ReadAssetDocumentInfo(ezUniquePtr<ezAssetDocumentInfo>& out_pInfo, ezStreamReader& stream) const
{
  ezAbstractObjectGraph graph;

  if (ezAbstractGraphDdlSerializer::ReadHeader(stream, &graph).Failed())
    return ezStatus("Failed to read asset document");

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");

  if (pHeaderNode == nullptr)
    return ezStatus("Document does not contain a 'Header'");

  ezAssetDocumentInfo* pEntry = static_cast<ezAssetDocumentInfo*>(rttiConverter.CreateObjectFromNode(pHeaderNode));
  EZ_ASSERT_DEBUG(pEntry != nullptr, "Failed to deserialize ezAssetDocumentInfo!");
  out_pInfo = ezUniquePtr<ezAssetDocumentInfo>(pEntry, ezFoundation::GetDefaultAllocator());
  return ezStatus(EZ_SUCCESS);
}

ezString ezAssetDocumentManager::GenerateResourceThumbnailPath(const char* szDocumentPath)
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);
  ;

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.Append(".jpg");

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

bool ezAssetDocumentManager::IsThumbnailUpToDate(const char* szDocumentPath, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  ezString sThumbPath = GenerateResourceThumbnailPath(szDocumentPath);
  ezFileReader file;
  if (file.Open(sThumbPath, 256).Failed())
    return false;

  char szTag[8];

  const ezUInt64 uiHeaderSize = ezAssetFileHeader::GetSerializedSize() + 7;
  ezUInt64 uiFileSize = file.GetFileSize();
  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);
  file.ReadBytes(szTag, 7);
  szTag[7] = '\0';
  if (!ezStringUtils::IsEqual(szTag, "ezThumb"))
    return false;

  ezAssetFileHeader assetHeader;
  assetHeader.Read(file);
  if (assetHeader.GetFileHash() != uiThumbnailHash)
    return false;
  if (assetHeader.GetFileVersion() != uiTypeVersion)
    return false;

  return true;
}

void ezAssetDocumentManager::AddEntriesToAssetTable(const char* szDataDirectory, const ezAssetProfile* pAssetProfile,
                                                    ezMap<ezString, ezString>& inout_GuidToPath) const
{
}

ezString ezAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezAssetProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(szDataDirectory, pSubAsset->m_pAssetInfo->m_sAbsolutePath, "", pAssetProfile);
}

ezString ezAssetDocumentManager::GetAbsoluteOutputFileName(const char* szDocumentPath, const char* szOutputTag,
                                                           const ezAssetProfile* pAssetProfile) const
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  ezString sRelativePath = GetRelativeOutputFileName(sProjectDir, szDocumentPath, szOutputTag, pAssetProfile);
  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag,
                                                           const ezAssetProfile* pAssetProfile) const
{
  const ezAssetProfile* sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(pAssetProfile);
  EZ_ASSERT_DEBUG(ezStringUtils::IsNullOrEmpty(szOutputTag),
                  "The output tag '%s' for '%s' is not supported, override GetRelativeOutputFileName", szOutputTag, szDocumentPath);
  ezStringBuilder sRelativePath(szDocumentPath);
  sRelativePath.MakeRelativeTo(szDataDirectory);
  GenerateOutputFilename(sRelativePath, sPlatform, GetResourceTypeExtension(), GeneratesPlatformSpecificAssets());

  return sRelativePath;
}

bool ezAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const ezSet<ezString>& outputs, ezUInt64 uiHash,
                                              ezUInt16 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  if (!IsOutputUpToDate(szDocumentPath, "", uiHash, uiTypeVersion))
    return false;

  for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
  {
    if (!IsOutputUpToDate(szDocumentPath, it.Key(), uiHash, uiTypeVersion))
      return false;
  }
  return true;
}

bool ezAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, ezUInt16 uiTypeVersion)
{
  const ezString sTargetFile = GetAbsoluteOutputFileName(szDocumentPath, szOutputTag);
  return ezAssetDocumentManager::IsResourceUpToDate(sTargetFile, uiHash, uiTypeVersion);
}

const ezAssetProfile* ezAssetDocumentManager::DetermineFinalTargetPlatform(const ezAssetProfile* pAssetProfile)
{
  if (pAssetProfile == nullptr)
  {
    return ezAssetCurator::GetSingleton()->GetActiveAssetProfile();
  }

  return pAssetProfile;
}

ezResult ezAssetDocumentManager::TryOpenAssetDocument(const char* szPathOrGuid)
{
  ezAssetCurator::ezLockedSubAsset pSubAsset;

  if (ezConversionUtils::IsStringUuid(szPathOrGuid))
  {
    ezUuid matGuid;
    matGuid = ezConversionUtils::ConvertStringToUuid(szPathOrGuid);

    pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way
    // either left as an exercise for whoever needs non-asset references
    pSubAsset = ezAssetCurator::GetSingleton()->FindSubAsset(szPathOrGuid);
  }

  if (pSubAsset)
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(pSubAsset->m_pAssetInfo->m_sAbsolutePath);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

bool ezAssetDocumentManager::IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion)
{
  CURATOR_PROFILE(szResourceFile);
  ezFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  // this might happen if writing to the file failed
  if (file.GetFileSize() == 0)
    return false;

  ezAssetFileHeader AssetHeader;
  AssetHeader.Read(file);

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

void ezAssetDocumentManager::GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const ezAssetProfile* pAssetProfile,
                                                    const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
    inout_sRelativeDocumentPath.Prepend(pAssetProfile->GetConfigName(), "/");
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
