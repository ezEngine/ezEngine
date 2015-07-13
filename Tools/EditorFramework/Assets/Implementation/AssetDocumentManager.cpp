#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, ezDocumentManagerBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

bool ezAssetDocumentManager::IsResourceUpToDate(ezUInt64 uiHash, const char* szResourceFile)
{
  ezFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  ezAssetFileHeader AssetHeader;
  AssetHeader.Read(file);

  return AssetHeader.IsFileUpToDate(uiHash);
}

ezString ezAssetDocumentManager::GenerateResourceFileName(const char* szDocumentPath, const char* szPlatform) const
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetInstance()->FindDataDirectoryForAsset(szDocumentPath);;

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", szPlatform, "/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GenerateResourceThumbnailPath(const char* szDocumentPath)
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetInstance()->FindDataDirectoryForAsset(szDocumentPath);;

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.ChangeFileExtension("jpg");

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;

}

ezString ezAssetDocumentManager::GenerateRelativeResourceFileName(const char* szDataDirectory, const char* szDocumentPath) const
{
  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(szDataDirectory);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());
  sRelativePath.MakeCleanPath();

  return sRelativePath;
}
