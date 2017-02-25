#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Core/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezBitflags<ezAssetDocumentFlags> ezAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::Default;
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

bool ezAssetDocumentManager::IsThumbnailUpToDate(const char* szDocumentPath, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion)
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

ezString ezAssetDocumentManager::GetAbsoluteOutputFileName(const char* szDocumentPath, const char* szOutputTag, const char* szPlatform) const
{
  ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  ezString sRelativePath = GetRelativeOutputFileName(sProjectDir, szDocumentPath, szOutputTag, szPlatform);
  ezStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

ezString ezAssetDocumentManager::GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const char* szPlatform) const
{
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  EZ_ASSERT_DEBUG(ezStringUtils::IsNullOrEmpty(szOutputTag), "The output tag '%s' for '%s' is not supported, override GetRelativeOutputFileName", szOutputTag, szDocumentPath);
  ezStringBuilder sRelativePath(szDocumentPath);
  sRelativePath.MakeRelativeTo(szDataDirectory);
  GenerateOutputFilename(sRelativePath, sPlatform, GetResourceTypeExtension(), GeneratesPlatformSpecificAssets());

  return sRelativePath;
}

bool ezAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const ezSet<ezString>& outputs, ezUInt64 uiHash, ezUInt16 uiTypeVersion)
{
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

ezString ezAssetDocumentManager::DetermineFinalTargetPlatform(const char* szPlatform)
{
  if (ezStringUtils::IsNullOrEmpty(szPlatform))
  {
    return ezAssetCurator::GetSingleton()->GetActivePlatform();
  }

  return szPlatform;
}

bool ezAssetDocumentManager::IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion)
{
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

void ezAssetDocumentManager::GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const char* szPlatform, const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
    inout_sRelativeDocumentPath.Prepend(szPlatform, "/");
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
