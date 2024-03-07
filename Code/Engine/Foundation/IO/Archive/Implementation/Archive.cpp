#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Logging/Log.h>

void operator<<(ezStreamWriter& inout_stream, const ezArchiveStoredString& value)
{
  inout_stream << value.m_uiLowerCaseHash;
  inout_stream << value.m_uiSrcStringOffset;
}

void operator>>(ezStreamReader& inout_stream, ezArchiveStoredString& value)
{
  inout_stream >> value.m_uiLowerCaseHash;
  inout_stream >> value.m_uiSrcStringOffset;
}

ezUInt32 ezArchiveTOC::FindEntry(ezStringView sFile) const
{
  ezStringBuilder sLowerCasePath = sFile;
  sLowerCasePath.ToLower();

  ezUInt32 uiIndex;

  ezArchiveLookupString lookup(ezHashingUtils::StringHash(sLowerCasePath.GetView()), sLowerCasePath, m_AllPathStrings);

  if (!m_PathToEntryIndex.TryGetValue(lookup, uiIndex))
    return ezInvalidIndex;

  EZ_ASSERT_DEBUG(sFile.IsEqual_NoCase(GetEntryPathString(uiIndex)), "Hash table corruption detected.");
  return uiIndex;
}

ezUInt32 ezArchiveTOC::AddPathString(ezStringView sPathString)
{
  const ezUInt32 offset = m_AllPathStrings.GetCount();
  const ezUInt32 numNewBytesNeeded = sPathString.GetElementCount() + 1;
  m_AllPathStrings.Reserve(m_AllPathStrings.GetCount() + numNewBytesNeeded);
  m_AllPathStrings.PushBackRange(ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(sPathString.GetStartPointer()), sPathString.GetElementCount()));
  m_AllPathStrings.PushBackUnchecked('\0');
  return offset;
}

void ezArchiveTOC::RebuildPathToEntryHashes()
{
  const ezUInt32 uiNumEntries = m_Entries.GetCount();
  m_PathToEntryIndex.Clear();
  m_PathToEntryIndex.Reserve(uiNumEntries);

  ezStringBuilder sLowerCasePath;

  for (ezUInt32 i = 0; i < uiNumEntries; i++)
  {
    const ezUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;
    ezStringView sEntryString = GetEntryPathString(i);
    sLowerCasePath = sEntryString;
    sLowerCasePath.ToLower();

    // cut off the upper 32 bit, we don't need them here
    const ezUInt32 uiLowerCaseHash = ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sLowerCasePath.GetView()) & 0xFFFFFFFFllu);

    m_PathToEntryIndex.Insert(ezArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

    // Verify that the conversion worked
    EZ_ASSERT_DEBUG(FindEntry(sEntryString) == i, "Hashed path retrieval did not yield inserted index");
  }
}

ezStringView ezArchiveTOC::GetEntryPathString(ezUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

ezResult ezArchiveTOC::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));

  // write the hash of a known string to the archive, to detect hash function changes
  ezUInt64 uiStringHash = ezHashingUtils::StringHash("ezArchive");
  inout_stream << uiStringHash;

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(m_PathToEntryIndex));

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_AllPathStrings));

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

ezResult ezArchiveTOC::Deserialize(ezStreamReader& inout_stream, ezUInt8 uiArchiveVersion)
{
  EZ_ASSERT_ALWAYS(uiArchiveVersion <= 4, "Unsupported archive version {}", uiArchiveVersion);

  // we don't use the TOC version anymore, but the archive version instead
  const ezTypeVersion version = inout_stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));

  bool bRecreateStringHashes = true;

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    ezHashTable<ezOldTempHashedString, ezUInt32> m_PathToIndex;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    if (uiArchiveVersion >= 4)
    {
      // read the hash of a known string from the archive, to detect hash function changes
      ezUInt64 uiStringHash = 0;
      inout_stream >> uiStringHash;

      if (uiStringHash == ezHashingUtils::StringHash("ezArchive"))
      {
        bRecreateStringHashes = false;
      }
    }

    EZ_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToEntryIndex));
  }

  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_AllPathStrings));

  if (bRecreateStringHashes)
  {
    ezLog::Info("Archive uses older string hashing, recomputing hashes.");

    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly
    //
    // version 2 used MurmurHash
    // version 3 switched to 32 bit xxHash
    // version 4 switched to 64 bit hashes

    RebuildPathToEntryHashes();
  }

  // path strings mustn't be empty and must be zero-terminated
  if (m_AllPathStrings.IsEmpty() || m_AllPathStrings.PeekBack() != '\0')
  {
    ezLog::Error("Archive is corrupt. Invalid string data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveEntry::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_uiDataStartOffset;
  inout_stream << m_uiUncompressedDataSize;
  inout_stream << m_uiStoredDataSize;
  inout_stream << (ezUInt8)m_CompressionMode;
  inout_stream << m_uiPathStringOffset;

  return EZ_SUCCESS;
}

ezResult ezArchiveEntry::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_uiDataStartOffset;
  inout_stream >> m_uiUncompressedDataSize;
  inout_stream >> m_uiStoredDataSize;
  ezUInt8 uiCompressionMode = 0;
  inout_stream >> uiCompressionMode;
  m_CompressionMode = (ezArchiveCompressionMode)uiCompressionMode;
  inout_stream >> m_uiPathStringOffset;

  return EZ_SUCCESS;
}


