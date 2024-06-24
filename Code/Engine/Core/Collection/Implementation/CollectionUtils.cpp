#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void ezCollectionUtils::AddFiles(ezCollectionResourceDescriptor& ref_collection, ezStringView sAssetTypeNameView, ezStringView sAbsPathToFolder, ezStringView sFileExtension, ezStringView sStripPrefix, ezStringView sPrependPrefix)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

  const ezUInt32 uiStripPrefixLength = ezStringUtils::GetCharacterCount(sStripPrefix.GetStartPointer(), sStripPrefix.GetEndPointer());

  ezFileSystemIterator fsIt;
  fsIt.StartSearch(sAbsPathToFolder, ezFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  ezStringBuilder sFullPath;
  ezHashedString sAssetTypeName;
  sAssetTypeName.Assign(sAssetTypeNameView);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (ezPathUtils::HasExtension(stats.m_sName, sFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(sPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry = ref_collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }

#else
  EZ_IGNORE_UNUSED(ref_collection);
  EZ_IGNORE_UNUSED(sAssetTypeNameView);
  EZ_IGNORE_UNUSED(sAbsPathToFolder);
  EZ_IGNORE_UNUSED(sFileExtension);
  EZ_IGNORE_UNUSED(sStripPrefix);
  EZ_IGNORE_UNUSED(sPrependPrefix);
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif
}


EZ_CORE_DLL void ezCollectionUtils::MergeCollections(ezCollectionResourceDescriptor& ref_result, ezArrayPtr<const ezCollectionResourceDescriptor*> inputCollections)
{
  ezMap<ezString, const ezCollectionEntry*> firstEntryOfID;

  for (const ezCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const ezCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        ref_result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


EZ_CORE_DLL void ezCollectionUtils::DeDuplicateEntries(ezCollectionResourceDescriptor& ref_result, const ezCollectionResourceDescriptor& input)
{
  const ezCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(ref_result, ezArrayPtr<const ezCollectionResourceDescriptor*>(&firstInput, 1));
}

void ezCollectionUtils::AddResourceHandle(ezCollectionResourceDescriptor& ref_collection, ezTypelessResourceHandle hHandle, ezStringView sAssetTypeName, ezStringView sAbsFolderpath)
{
  if (!hHandle.IsValid())
    return;

  const char* resID = hHandle.GetResourceID();

  auto& entry = ref_collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(sAssetTypeName);
  entry.m_sResourceID = resID;

  ezStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (!sAbsFolderpath.IsEmpty())
  {
    ezStringView root, relFile;
    ezPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = sAbsFolderpath;
    absFilename.AppendPath(relFile.GetStartPointer());
    absFilename.MakeCleanPath();

    ezFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && ezFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}
