#include <FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>

ezUInt32 ezArchiveTOC::FindEntry(const char* szFile) const
{
  ezStringBuilder sPath = szFile;
  sPath.ToLower();

  ezUInt32 uiIndex;
  if (!m_PathToIndex.TryGetValue(ezTempHashedString(sPath.GetData()), uiIndex))
    return ezInvalidIndex;

  return uiIndex;
}

const char* ezArchiveTOC::GetEntryPathString(ezUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

ezResult ezArchiveTOC::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Entries));

  EZ_SUCCEED_OR_RETURN(stream.WriteHashTable(m_PathToIndex));

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_AllPathStrings));

  return EZ_SUCCESS;
}

ezResult ezArchiveTOC::Deserialize(ezStreamReader& stream)
{
  ezTypeVersion version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Entries));

  EZ_SUCCEED_OR_RETURN(stream.ReadHashTable(m_PathToIndex));

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_AllPathStrings));

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
