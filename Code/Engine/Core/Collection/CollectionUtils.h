#pragma once

#include <Core/Collection/CollectionResource.h>

class ezHashedString;

namespace ezCollectionUtils
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) || defined(EZ_DOCS)
  /// \brief Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see ezResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  EZ_CORE_DLL void AddFiles(ezCollectionResourceDescriptor& collection, const char* szAssetTypeName, const char* szAbsPathToFolder, const char* szFileExtension, const char* szStripPrefix, const char* szPrependPrefix);
#endif
};
