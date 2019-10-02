#include <FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>

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

  ezArchiveLookupString lookup(ezTempHashedString::ComputeHash(sLowerCasePath.GetData()), sLowerCasePath, m_AllPathStrings);

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

  // version 2 changed the way the hash table is generated
  EZ_SUCCEED_OR_RETURN(stream.WriteHashTable(m_PathToEntryIndex));

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_AllPathStrings));

  return EZ_SUCCESS;
}

ezResult ezArchiveTOC::Deserialize(ezStreamReader& stream)
{
  ezTypeVersion version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Entries));

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    ezHashTable<ezTempHashedString, ezUInt32> m_PathToIndex;
    EZ_SUCCEED_OR_RETURN(stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    EZ_SUCCEED_OR_RETURN(stream.ReadHashTable(m_PathToEntryIndex));
  }

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_AllPathStrings));

  if (version == 1)
  {
    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly

    const ezUInt32 uiNumEntries = m_Entries.GetCount();
    m_PathToEntryIndex.Reserve(uiNumEntries);

    ezStringBuilder sLowerCasePath;

    for (ezUInt32 i = 0; i < uiNumEntries; i++)
    {
      const ezUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;

      const char* szEntryString = GetEntryPathString(i);

      sLowerCasePath = szEntryString;
      sLowerCasePath.ToLower();

      const ezUInt32 uiLowerCaseHash = ezTempHashedString::ComputeHash(sLowerCasePath.GetData());

      m_PathToEntryIndex.Insert(ezArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

      // Verify that the conversion worked
      EZ_ASSERT_DEBUG(FindEntry(szEntryString) == i, "Hashed path retrieval did not yield inserted index");
    }
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
