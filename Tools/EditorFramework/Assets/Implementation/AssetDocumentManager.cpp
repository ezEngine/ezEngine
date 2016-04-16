#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

bool ezAssetDocumentManager::IsResourceUpToDate(ezUInt64 uiHash, ezUInt16 uiTypeVersion, const char* szResourceFile)
{
  ezFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  ezAssetFileHeader AssetHeader;
  AssetHeader.Read(file);

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
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
  sRelativePath.ChangeFileExtension("jpg");

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;

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
