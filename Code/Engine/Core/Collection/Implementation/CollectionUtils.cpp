#include <CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

void ezCollectionUtils::AddFiles(ezCollectionResourceDescriptor& collection, const char* szAssetTypeName, const char* szAbsPathToFolder,
  const char* szFileExtension, const char* szStripPrefix, const char* szPrependPrefix)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezFileSystemIterator fsIt;

  const ezUInt32 uiStripPrefixLength = ezStringUtils::GetCharacterCount(szStripPrefix);

  if (fsIt.StartSearch(szAbsPathToFolder, ezFileSystemIteratorFlags::ReportFilesRecursive).Failed())
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
      entry.m_uiFileSize = stats.m_uiFileSize;
    }

  } while (fsIt.Next().Succeeded());
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif
}


EZ_CORE_DLL void ezCollectionUtils::MergeCollections(
  ezCollectionResourceDescriptor& result, ezArrayPtr<const ezCollectionResourceDescriptor*> inputCollections)
{
  ezMap<ezString, const ezCollectionEntry*> firstEntryOfID;

  for (const ezCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const ezCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


EZ_CORE_DLL void ezCollectionUtils::DeDuplicateEntries(ezCollectionResourceDescriptor& result, const ezCollectionResourceDescriptor& input)
{
  const ezCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(result, ezArrayPtr<const ezCollectionResourceDescriptor*>(&firstInput, 1));
}

void ezCollectionUtils::AddResourceHandle(
  ezCollectionResourceDescriptor& collection, ezTypelessResourceHandle handle, const char* szAssetTypeName, const char* szAbsFolderpath)
{
  if (!handle.IsValid())
    return;

  const char* resID = handle.GetResourceID();

  auto& entry = collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(szAssetTypeName);
  entry.m_sResourceID = resID;

  ezStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (szAbsFolderpath != nullptr)
  {
    ezStringView root, relFile;
    ezPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = szAbsFolderpath;
    absFilename.AppendPath(relFile.GetStartPointer());
    absFilename.MakeCleanPath();

    ezFileStats stats;
    if (!absFilename.IsEmpty()
      && absFilename.IsAbsolutePath()
      && ezFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionUtils);
