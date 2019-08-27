#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

class ezRawMemoryStreamReader;

/// \brief Compression modes for ezArchive file entries
enum class ezArchiveCompressionMode : ezUInt8
{
  Uncompressed,
  Compressed_zstd,
  Compressed_zip,
};

/// \brief Data for a single file entry in an ezArchive file
class EZ_FOUNDATION_DLL ezArchiveEntry
{
public:
  ezUInt64 m_uiDataStartOffset = 0;      ///< Byte offset for where the file's (compressed) data stream starts in the ezArchive
  ezUInt64 m_uiUncompressedDataSize = 0; ///< Size of the original uncompressed data.
  ezUInt64 m_uiStoredDataSize = 0;       ///< The amount of (compressed) bytes actually stored in the ezArchive.
  ezUInt32 m_uiPathStringOffset = 0;     ///< Byte offset into ezArchiveTOC::m_AllPathStrings where the path string for this entry resides.
  ezArchiveCompressionMode m_CompressionMode = ezArchiveCompressionMode::Uncompressed;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

/// \brief Table-of-contents for an ezArchive file
class EZ_FOUNDATION_DLL ezArchiveTOC
{
public:
  /// all files stored in the ezArchive
  ezDynamicArray<ezArchiveEntry> m_Entries;
  /// allows to map a hashed string to the index of the file entry for the file path
  ezHashTable<ezTempHashedString, ezUInt32> m_PathToIndex;
  /// one large array holding all path strings for the file entries, to reduce allocations
  ezDynamicArray<ezUInt8> m_AllPathStrings;

  /// \brief Returns the entry index for the given file or ezInvalidIndex, if not found.
  ezUInt32 FindEntry(const char* szFile) const;

  const char* GetEntryPathString(ezUInt32 uiEntryIdx) const;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};


