#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocumentManager::ezAssetDocumentManager() = default;
ezAssetDocumentManager::~ezAssetDocumentManager() = default;

ezStatus ezAssetDocumentManager::CloneDocument(ezStringView sPath, ezStringView sClonePath, ezUuid& inout_cloneGuid)
{
  ezStatus res = SUPER::CloneDocument(sPath, sClonePath, inout_cloneGuid);
  if (res.Succeeded())
  {
    // Cloned documents are usually opened right after cloning. To make sure this does not fail we need to inform the asset curator of the newly added asset document.
    ezAssetCurator::GetSingleton()->NotifyOfFileChange(sClonePath);
  }
  return res;
}

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

ezStatus ezAssetDocumentManager::ReadAssetDocumentInfo(ezUniquePtr<ezAssetDocumentInfo>& out_pInfo, ezStreamReader& inout_stream) const
{
  ezAbstractObjectGraph graph;

  if (ezAbstractGraphDdlSerializer::ReadHeader(inout_stream, &graph).Failed())
    return ezStatus("Failed to read asset document");

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");

  if (pHeaderNode == nullptr)
    return ezStatus("Document does not contain a 'Header'");

  ezAssetDocumentInfo* pEntry = rttiConverter.CreateObjectFromNode(pHeaderNode).Cast<ezAssetDocumentInfo>();
  EZ_ASSERT_DEBUG(pEntry != nullptr, "Failed to deserialize ezAssetDocumentInfo!");
  out_pInfo = ezUniquePtr<ezAssetDocumentInfo>(pEntry, ezFoundation::GetDefaultAllocator());
  return ezStatus(EZ_SUCCESS);
}

ezString ezAssetDocumentManager::GenerateResourceThumbnailPath(ezStringView sDocumentPath, ezStringView sSubAssetName)
{
  ezStringBuilder sRelativePath;
  if (sSubAssetName.IsEmpty())
  {
    sRelativePath = sDocumentPath;
  }
  else
  {
    sRelativePath = sDocumentPath.GetFileDirectory();

    ezStringBuilder sValidFileName;
    ezPathUtils::MakeValidFilename(sSubAssetName, '_', sValidFileName);
    sRelativePath.AppendPath(sValidFileName);
  }

  ezString sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sRelativePath);

  sRelativePath.MakeRelativeTo(sProjectDir).IgnoreResult();
  sRelativePath.Append(".jpg");

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

bool ezAssetDocumentManager::IsThumbnailUpToDate(ezStringView sDocumentPath, ezStringView sSubAssetName, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  ezString sThumbPath = GenerateResourceThumbnailPath(sDocumentPath, sSubAssetName);
  ezFileReader file;
  if (file.Open(sThumbPath, 256).Failed())
    return false;

  ezAssetDocument::ThumbnailInfo thumbnailInfo;

  const ezUInt64 uiHeaderSize = thumbnailInfo.GetSerializedSize();
  ezUInt64 uiFileSize = file.GetFileSize();

  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);

  if (thumbnailInfo.Deserialize(file).Failed())
  {
    return false;
  }

  return thumbnailInfo.IsThumbnailUpToDate(uiThumbnailHash, uiTypeVersion);
}

void ezAssetDocumentManager::AddEntriesToAssetTable(ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile, ezDelegate<void(ezStringView sGuid, ezStringView sPath, ezStringView sType)> addEntry) const {}

ezString ezAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor, sDataDirectory, pSubAsset->m_pAssetInfo->m_Path, "", pAssetProfile);
}

ezString ezAssetDocumentManager::GetAbsoluteOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sDocumentPath, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile) const
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sDocumentPath);

  ezString sRelativePath = GetRelativeOutputFileName(pTypeDesc, sProjectDir, sDocumentPath, sOutputTag, pAssetProfile);
  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sDataDirectory, ezStringView sDocumentPath, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile) const
{
  const ezPlatformProfile* pPlatform = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
  EZ_ASSERT_DEBUG(sOutputTag.IsEmpty(), "The output tag '{}' for '{}' is not supported, override GetRelativeOutputFileName", sOutputTag, sDocumentPath);

  ezStringBuilder sRelativePath(sDocumentPath);
  sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
  GenerateOutputFilename(sRelativePath, pPlatform, pTypeDesc->m_sResourceFileExtension, GeneratesProfileSpecificAssets());

  return sRelativePath;
}

bool ezAssetDocumentManager::IsOutputUpToDate(ezStringView sDocumentPath, const ezDynamicArray<ezString>& outputs, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  CURATOR_PROFILE(sDocumentPath);
  if (!IsOutputUpToDate(sDocumentPath, "", uiHash, pTypeDescriptor))
    return false;

  for (const ezString& sOutput : outputs)
  {
    if (!IsOutputUpToDate(sDocumentPath, sOutput, uiHash, pTypeDescriptor))
      return false;
  }
  return true;
}

bool ezAssetDocumentManager::IsOutputUpToDate(ezStringView sDocumentPath, ezStringView sOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  const ezString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, sDocumentPath, sOutputTag);
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
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
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
  AssetHeader.Read(file).IgnoreResult();

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

void ezAssetDocumentManager::GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const ezPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
  {
    const ezPlatformProfile* pPlatform = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
    inout_sRelativeDocumentPath.Prepend(pPlatform->GetConfigName(), "/");
  }
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
