#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezBitflags<ezAssetDocumentFlags> ezAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::Default;
}

bool ezAssetDocumentManager::IsResourceUpToDate(ezUInt64 uiHash, ezUInt16 uiTypeVersion, const char* szResourceFile)
{
  ezFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  ezAssetFileHeader AssetHeader;
  AssetHeader.Read(file);

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

bool ezAssetDocumentManager::IsThumbnailUpToDate(ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion, const char* szDocumentPath)
{
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

ezString ezAssetDocumentManager::GenerateResourceFileName(const char* szDocumentPath, const char* szPlatform) const
{
  EZ_ASSERT_DEBUG(!ezStringUtils::IsNullOrEmpty(szPlatform), "Platform string must be set");

  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  ezString sPlatform;
  if (GeneratesPlatformSpecificAssets()) /// \todo Put this into ezAssetDocumentFlag 
    sPlatform = szPlatform;
  else
    sPlatform = "Common";

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sPlatform, "/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GenerateResourceThumbnailPath(const char* szDocumentPath)
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);;

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.Append(".jpg");

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;

}

ezString ezAssetDocumentManager::GetFinalOutputFileName(const ezDocumentTypeDescriptor* pDescriptor, const char* szDocumentPath, const char* szPlatform) const
{
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  return GenerateResourceFileName(szDocumentPath, sPlatform);
}

ezString ezAssetDocumentManager::GenerateRelativeResourceFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szPlatform) const
{
  EZ_ASSERT_DEBUG(!ezStringUtils::IsNullOrEmpty(szPlatform), "Platform string must be set");

  ezStringBuilder sRelativePath(szDocumentPath);

  sRelativePath.MakeRelativeTo(szDataDirectory);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());
  sRelativePath.MakeCleanPath();

  if (GeneratesPlatformSpecificAssets())
    sRelativePath.Prepend(szPlatform, "/");
  else
    sRelativePath.Prepend("Common/");

  return sRelativePath;
}

ezString ezAssetDocumentManager::DetermineFinalTargetPlatform(const char* szPlatform)
{
  if (ezStringUtils::IsNullOrEmpty(szPlatform))
  {
    return ezAssetCurator::GetSingleton()->GetActivePlatform();
  }

  return szPlatform;
}

