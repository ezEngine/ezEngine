#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Logging/Log.h>

void operator<<(ezStreamWriter& stream, const ezArchiveStoredString& value)
{
  stream << value.m_uiLowerCaseHash;
  stream << value.m_uiSrcStringOffset;
}

void operator>>(ezStreamReader& stream, ezArchiveStoredString& value)
{
  stream >> value.m_uiLowerCaseHash;
  stream >> value.m_uiSrcStringOffset;
}

ezUInt32 ezArchiveTOC::FindEntry(const char* szFile) const
{
  ezStringBuilder sLowerCasePath = szFile;
  sLowerCasePath.ToLower();

  ezUInt32 uiIndex;

  ezArchiveLookupString lookup(ezHashingUtils::StringHash(sLowerCasePath.GetView()), sLowerCasePath, m_AllPathStrings);

  if (!m_PathToEntryIndex.TryGetValue(lookup, uiIndex))
    return ezInvalidIndex;

  EZ_ASSERT_DEBUG(ezStringUtils::IsEqual_NoCase(szFile, GetEntryPathString(uiIndex)), "Hash table corruption detected.");
  return uiIndex;
}

const char* ezArchiveTOC::GetEntryPathString(ezUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

ezResult ezArchiveTOC::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Entries));

  // write the hash of a known string to the archive, to detect hash function changes
  ezUInt64 uiStringHash = ezHashingUtils::StringHash("ezArchive");
  stream << uiStringHash;

  EZ_SUCCEED_OR_RETURN(stream.WriteHashTable(m_PathToEntryIndex));

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_AllPathStrings));

  return EZ_SUCCESS;
}

struct ezOldTempHashedString
{
  ezUInt32 m_uiHash = 0;

  ezResult Deserialize(ezStreamReader& r)
  {
    r >> m_uiHash;
    return EZ_SUCCESS;
  }

  bool operator==(const ezOldTempHashedString& rhs) const
  {
    return m_uiHash == rhs.m_uiHash;
  }
};

template <>
struct ezHashHelper<ezOldTempHashedString>
{
  static ezUInt32 Hash(const ezOldTempHashedString& value)
  {
    return value.m_uiHash;
  }

  static bool Equal(const ezOldTempHashedString& a, const ezOldTempHashedString& b) { return a == b; }
};

ezResult ezArchiveTOC::Deserialize(ezStreamReader& stream, ezUInt8 uiArchiveVersion)
{
  EZ_ASSERT_ALWAYS(uiArchiveVersion <= 4, "Unsupported archive version {}", uiArchiveVersion);

  // we don't use the TOC version anymore, but the archive version instead
  const ezTypeVersion version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Entries));

  bool bRecreateStringHashes = true;

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    ezHashTable<ezOldTempHashedString, ezUInt32> m_PathToIndex;
    EZ_SUCCEED_OR_RETURN(stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    if (uiArchiveVersion >= 4)
    {
      // read the hash of a known string from the archive, to detect hash function changes
      ezUInt64 uiStringHash = 0;
      stream >> uiStringHash;

      if (uiStringHash == ezHashingUtils::StringHash("ezArchive"))
      {
        bRecreateStringHashes = false;
      }
    }

    EZ_SUCCEED_OR_RETURN(stream.ReadHashTable(m_PathToEntryIndex));
  }

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_AllPathStrings));

  if (bRecreateStringHashes)
  {
    ezLog::Info("Archive uses older string hashing, recomputing hashes.");

    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly
    //
    // version 2 used MurmurHash
    // version 3 switched to 32 bit xxHash
    // version 4 switched to 64 bit hashes

    const ezUInt32 uiNumEntries = m_Entries.GetCount();
    m_PathToEntryIndex.Clear();
    m_PathToEntryIndex.Reserve(uiNumEntries);

    ezStringBuilder sLowerCasePath;

    for (ezUInt32 i = 0; i < uiNumEntries; i++)
    {
      const ezUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;

      const char* szEntryString = GetEntryPathString(i);

      sLowerCasePath = szEntryString;
      sLowerCasePath.ToLower();

      // cut off the upper 32 bit, we don't need them here
      const ezUInt32 uiLowerCaseHash = ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sLowerCasePath.GetView()) & 0xFFFFFFFFllu);

      m_PathToEntryIndex.Insert(ezArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

      // Verify that the conversion worked
      EZ_ASSERT_DEBUG(FindEntry(szEntryString) == i, "Hashed path retrieval did not yield inserted index");
    }
  }

  // path strings mustn't be empty and must be zero-terminated
  if (m_AllPathStrings.IsEmpty() || m_AllPathStrings.PeekBack() != '\0')
  {
    ezLog::Error("Archive is corrupt. Invalid string data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveEntry::Serialize(ezStreamWriter& stream) const
{
  stream << m_uiDataStartOffset;
  stream << m_uiUncompressedDataSize;
  stream << m_uiStoredDataSize;
  stream << (ezUInt8)m_CompressionMode;
  stream << m_uiPathStringOffset;

  return EZ_SUCCESS;
}

ezResult ezArchiveEntry::Deserialize(ezStreamReader& stream)
{
  stream >> m_uiDataStartOffset;
  stream >> m_uiUncompressedDataSize;
  stream >> m_uiStoredDataSize;
  ezUInt8 uiCompressionMode = 0;
  stream >> uiCompressionMode;
  m_CompressionMode = (ezArchiveCompressionMode)uiCompressionMode;
  stream >> m_uiPathStringOffset;

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_Archive);
