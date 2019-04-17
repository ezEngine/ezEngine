#include <CorePCH.h>

#include <Foundation/IO/OSFile.h>
#include <Core/Collection/CollectionUtils.h>


void ezCollectionUtils::AddFiles(ezCollectionResourceDescriptor& collection, const char* szAssetTypeName,
  const char* szAbsPathToFolder, const char* szFileExtension, const char* szStripPrefix, const char* szPrependPrefix)
{
  ezFileSystemIterator fsIt;

  const ezUInt32 uiStripPrefixLength = ezStringUtils::GetCharacterCount(szStripPrefix);

  if (fsIt.StartSearch(szAbsPathToFolder, true, false).Failed())
    return;

  ezStringBuilder sFullPath;
  ezHashedString sAssetTypeName;
  sAssetTypeName.Assign(szAssetTypeName);

  do
  {
    const auto& stats = fsIt.GetStats();

    if (ezPathUtils::HasExtension(stats.m_sName, szFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(szPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry = collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
    }

  } while (fsIt.Next().Succeeded());
}

