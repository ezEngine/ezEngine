#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, ezDocumentManagerBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

bool ezAssetDocumentManager::IsResourceUpToDate(ezUInt64 uiHash, const char* szResourceFile)
{
  ezFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  ezUInt64 uiHashInFile = 0;
  file >> uiHashInFile;

  return uiHash == uiHashInFile;
}

ezString ezAssetDocumentManager::GenerateResourceFileName(const char* szDocumentPath, const char* szPlatform) const
{
  ezStringBuilder sProjectDir = ezToolsProject::GetInstance()->GetProjectPath();
  sProjectDir.PathParentDirectory();

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());

  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", szPlatform, "/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GenerateRelativeResourceFileName(const char* szDocumentPath) const
{
  ezStringBuilder sProjectDir = ezToolsProject::GetInstance()->GetProjectPath();
  sProjectDir.PathParentDirectory();

  ezStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir);
  sRelativePath.ChangeFileExtension(GetResourceTypeExtension());
  sRelativePath.MakeCleanPath();

  return sRelativePath;
}
