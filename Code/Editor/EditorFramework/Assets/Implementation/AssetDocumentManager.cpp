#include <EditorFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <EditorFramework/Assets/AssetDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocumentManager::ezAssetDocumentManager() = default;
ezAssetDocumentManager::~ezAssetDocumentManager() = default;

void ezAssetDocumentManager::ComputeAssetProfileHash(const ezPlatformProfile* pAssetProfile)
{
  m_uiAssetProfileHash = ComputeAssetProfileHashImpl(DetermineFinalTargetProfile(pAssetProfile));

  if (GeneratesProfileSpecificAssets())
  {
    EZ_ASSERT_DEBUG(m_uiAssetProfileHash != 0, "Assets that generate a profile-specific output must compute a hash for the profile settings.");
  }
  else
  {
    EZ_ASSERT_DEBUG(m_uiAssetProfileHash == 0, "Only assets that generate per-profile outputs may specify an asset profile hash.");
    m_uiAssetProfileHash = 0;
  }
}

ezUInt64 ezAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  return 0;
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

  ezAssetDocument::ThumbnailInfo thumbnailInfo;

  const ezUInt64 uiHeaderSize = thumbnailInfo.GetSerializedSize();
  ezUInt64 uiFileSize = file.GetFileSize();

  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);

  if(thumbnailInfo.Deserialize(file).Failed())
  {
    return false;
  }

  return thumbnailInfo.IsThumbnailUpToDate(uiThumbnailHash, uiTypeVersion);
}

void ezAssetDocumentManager::AddEntriesToAssetTable(const char* szDataDirectory, const ezPlatformProfile* pAssetProfile,
                                                    ezMap<ezString, ezString>& inout_GuidToPath) const
{
}

ezString ezAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor, szDataDirectory, pSubAsset->m_pAssetInfo->m_sAbsolutePath, "", pAssetProfile);
}

ezString ezAssetDocumentManager::GetAbsoluteOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, const char* szDocumentPath, const char* szOutputTag, const ezPlatformProfile* pAssetProfile) const
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  ezString sRelativePath = GetRelativeOutputFileName(pTypeDesc, sProjectDir, szDocumentPath, szOutputTag, pAssetProfile);
  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const ezPlatformProfile* pAssetProfile) const
{
  const ezPlatformProfile* sPlatform = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
  EZ_ASSERT_DEBUG(ezStringUtils::IsNullOrEmpty(szOutputTag), "The output tag '%s' for '%s' is not supported, override GetRelativeOutputFileName", szOutputTag, szDocumentPath);

  ezStringBuilder sRelativePath(szDocumentPath);
  sRelativePath.MakeRelativeTo(szDataDirectory);
  GenerateOutputFilename(sRelativePath, sPlatform, pTypeDesc->m_sResourceFileExtension, GeneratesProfileSpecificAssets());

  return sRelativePath;
}

bool ezAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const ezSet<ezString>& outputs, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  CURATOR_PROFILE(szDocumentPath);
  if (!IsOutputUpToDate(szDocumentPath, "", uiHash, pTypeDescriptor))
    return false;

  for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
  {
    if (!IsOutputUpToDate(szDocumentPath, it.Key(), uiHash, pTypeDescriptor))
      return false;
  }
  return true;
}

bool ezAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  const ezString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, szDocumentPath, szOutputTag);
  return ezAssetDocumentManager::IsResourceUpToDate(sTargetFile, uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());
}

const ezPlatformProfile* ezAssetDocumentManager::DetermineFinalTargetProfile(const ezPlatformProfile* pAssetProfile)
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

void ezAssetDocumentManager::GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const ezPlatformProfile* pAssetProfile,
                                                    const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
    inout_sRelativeDocumentPath.Prepend(pAssetProfile->GetConfigName(), "/");
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
