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

/// \brief Helper class to store a hashed string for quick lookup in the archive TOC
///
/// Stores a hash of the lower case string for quick comparison.
/// Additionally stores an offset into the ezArchiveTOC::m_AllPathStrings array for final validation, to prevent hash collisions.
/// The proper string lookup with hash collision check only works together with ezArchiveLookupString, which has the necessary context
/// to index the ezArchiveTOC::m_AllPathStrings array.
class EZ_FOUNDATION_DLL ezArchiveStoredString
{
public:
  EZ_DECLARE_POD_TYPE();

  ezArchiveStoredString() = default;

  ezArchiveStoredString(ezUInt32 uiLowerCaseHash, ezUInt32 uiSrcStringOffset)
    : m_uiLowerCaseHash(uiLowerCaseHash)
    , m_uiSrcStringOffset(uiSrcStringOffset)
  {
  }

  ezUInt32 m_uiLowerCaseHash;
  ezUInt32 m_uiSrcStringOffset;
};

void operator<<(ezStreamWriter& stream, const ezArchiveStoredString& value);
void operator>>(ezStreamReader& stream, ezArchiveStoredString& value);

/// \brief Helper class for looking up path strings in ezArchiveTOC::FindEntry()
///
/// Only works together with ezArchiveStoredString.
class ezArchiveLookupString
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezArchiveLookupString);

public:
  EZ_DECLARE_POD_TYPE();

  ezArchiveLookupString(ezUInt32 uiLowerCaseHash, const char* string, const ezDynamicArray<ezUInt8>& ArchiveAllPathStrings)
    : m_uiLowerCaseHash(uiLowerCaseHash)
    , m_szString(string)
    , m_ArchiveAllPathStrings(ArchiveAllPathStrings)
  {
  }

  ezUInt32 m_uiLowerCaseHash;
  const char* m_szString = nullptr;
  const ezDynamicArray<ezUInt8>& m_ArchiveAllPathStrings;
};

/// \brief Functions to enable ezHashTable to 1) store ezArchiveStoredString and 2) lookup strings efficiently with a ezArchiveLookupString
template <>
struct ezHashHelper<ezArchiveStoredString>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezArchiveStoredString& hs) { return hs.m_uiLowerCaseHash; }
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezArchiveLookupString& hs) { return hs.m_uiLowerCaseHash; }

  EZ_ALWAYS_INLINE static bool Equal(const ezArchiveStoredString& a, const ezArchiveStoredString& b)
  {
    return a.m_uiSrcStringOffset == b.m_uiSrcStringOffset;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezArchiveStoredString& a, const ezArchiveLookupString& b)
  {
    // in case that we want to lookup a string using a ezArchiveLookupString, we validate
    // that the stored string is actually equal to the lookup string, to enable handling of hash collisions
    return ezStringUtils::IsEqual_NoCase(reinterpret_cast<const char*>(&b.m_ArchiveAllPathStrings[a.m_uiSrcStringOffset]), b.m_szString);
  }
};

/// \brief Table-of-contents for an ezArchive file
class EZ_FOUNDATION_DLL ezArchiveTOC
{
public:
  /// all files stored in the ezArchive
  ezDynamicArray<ezArchiveEntry> m_Entries;
  /// allows to map a hashed string to the index of the file entry for the file path
  ezHashTable<ezArchiveStoredString, ezUInt32> m_PathToEntryIndex;
  /// one large array holding all path strings for the file entries, to reduce allocations
  ezDynamicArray<ezUInt8> m_AllPathStrings;

  /// \brief Returns the entry index for the given file or ezInvalidIndex, if not found.
  ezUInt32 FindEntry(const char* szFile) const;

  const char* GetEntryPathString(ezUInt32 uiEntryIdx) const;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};
