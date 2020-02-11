#pragma once

#include <Core/Collection/CollectionResource.h>

class ezHashedString;

namespace ezCollectionUtils
{
  /// \brief Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see ezResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  EZ_CORE_DLL void AddFiles(ezCollectionResourceDescriptor& collection, const char* szAssetTypeName, const char* szAbsPathToFolder, const char* szFileExtension, const char* szStripPrefix, const char* szPrependPrefix);

  /// \brief Merges all collections from the input array into the target result collection. Resource entries will be de-duplicated by resource ID string.
  EZ_CORE_DLL void MergeCollections(
    ezCollectionResourceDescriptor& result, ezArrayPtr<const ezCollectionResourceDescriptor*> inputCollections);

  /// \brief Special case of ezCollectionUtils::MergeCollections which outputs unique entries from input collection into the result collection 
  EZ_CORE_DLL void DeDuplicateEntries(ezCollectionResourceDescriptor& result, const ezCollectionResourceDescriptor& input);

  /// \brief Extracts info (i.e. resource ID as file path) from the passed handle and adds it as a new resource entry. Does not add an entry if the resource handle is not valid.
  ///
  /// The resource type identifier must be passed explicity as szAssetTypeName (see ezResourceManager::RegisterResourceForAssetType). To determine the
  /// file size, the resource ID is used as a filename passed to ezFileSystem::GetFileStats. In case the resource's path root is not mounted, the path root can be replaced by passing non-NULL
  /// string to szAbsFolderpath, which will replace the root, e.g. with an absolute file path. This is just for the file size check within the scope of the function, it will not modify the resource Id.
  EZ_CORE_DLL void AddResourceHandle(
    ezCollectionResourceDescriptor& collection, ezTypelessResourceHandle handle, const char* szAssetTypeName, const char* szAbsFolderpath);

};
