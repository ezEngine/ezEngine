#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/ChunkStream.h>

ezChunkStreamWriter::ezChunkStreamWriter(ezStreamWriter& inout_stream)
  : m_Stream(inout_stream)
{
  m_bWritingFile = false;
  m_bWritingChunk = false;
}

void ezChunkStreamWriter::BeginStream(ezUInt16 uiVersion)
{
  EZ_ASSERT_DEV(!m_bWritingFile, "Already writing the file.");
  EZ_ASSERT_DEV(uiVersion > 0, "The version number must be larger than 0");

  m_bWritingFile = true;

  const char* szTag = "BGNCHNK2";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
  m_Stream.WriteBytes(&uiVersion, 2).IgnoreResult();
}

void ezChunkStreamWriter::EndStream()
{
  EZ_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT_DEV(!m_bWritingChunk, "A chunk is still open for writing: '{0}'", m_sChunkName);

  m_bWritingFile = false;

  const char* szTag = "END CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
}

void ezChunkStreamWriter::BeginChunk(ezStringView sName, ezUInt32 uiVersion)
{
  EZ_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT_DEV(!m_bWritingChunk, "A chunk is already open for writing: '{0}'", m_sChunkName);

  m_sChunkName = sName;

  const char* szTag = "NXT CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();

  m_Stream << m_sChunkName;
  m_Stream << uiVersion;

  m_bWritingChunk = true;
}


void ezChunkStreamWriter::EndChunk()
{
  EZ_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT_DEV(m_bWritingChunk, "No chunk is currently open.");

  m_bWritingChunk = false;

  const ezUInt32 uiStorageSize = m_Storage.GetCount();
  m_Stream << uiStorageSize;
  /// \todo Write Chunk CRC

  for (ezUInt32 i = 0; i < uiStorageSize;)
  {
    const ezUInt32 uiRange = m_Storage.GetContiguousRange(i);

    EZ_ASSERT_DEBUG(uiRange > 0, "Invalid contiguous range");

    m_Stream.WriteBytes(&m_Storage[i], uiRange).IgnoreResult();
    i += uiRange;
  }

  m_Storage.Clear();
}

ezResult ezChunkStreamWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_bWritingChunk, "No chunk is currently written to");

  const ezUInt8* pBytes = (const ezUInt8*)pWriteBuffer;

  for (ezUInt64 i = 0; i < uiBytesToWrite; ++i)
    m_Storage.PushBack(pBytes[i]);

  return EZ_SUCCESS;
}



ezChunkStreamReader::ezChunkStreamReader(ezStreamReader& inout_stream)
  : m_Stream(inout_stream)
{
  m_ChunkInfo.m_bValid = false;
  m_EndChunkFileMode = EndChunkFileMode::JustClose;
}

ezUInt64 ezChunkStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  EZ_ASSERT_DEV(m_ChunkInfo.m_bValid, "No valid chunk available.");

  uiBytesToRead = ezMath::Min<ezUInt64>(uiBytesToRead, m_ChunkInfo.m_uiUnreadChunkBytes);
  m_ChunkInfo.m_uiUnreadChunkBytes -= (ezUInt32)uiBytesToRead;

  return m_Stream.ReadBytes(pReadBuffer, uiBytesToRead);
}

ezUInt16 ezChunkStreamReader::BeginStream()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  ezUInt16 uiVersion = 0;

  if (ezStringUtils::IsEqual(szTag, "BGNCHNK2"))
  {
    m_Stream.ReadBytes(&uiVersion, 2);
  }
  else
  {
    // "BGN CHNK" is the old chunk identifier, before a version number was written
    EZ_ASSERT_DEV(ezStringUtils::IsEqual(szTag, "BGN CHNK"), "Not a valid chunk file.");
  }

  TryReadChunkHeader();
  return uiVersion;
}

void ezChunkStreamReader::EndStream()
{
  if (m_EndChunkFileMode == EndChunkFileMode::SkipToEnd)
  {
    while (m_ChunkInfo.m_bValid)
      NextChunk();
  }
}

void ezChunkStreamReader::TryReadChunkHeader()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  if (ezStringUtils::IsEqual(szTag, "END CHNK"))
    return;

  if (ezStringUtils::IsEqual(szTag, "NXT CHNK"))
  {
    m_Stream >> m_ChunkInfo.m_sChunkName;
    m_Stream >> m_ChunkInfo.m_uiChunkVersion;
    m_Stream >> m_ChunkInfo.m_uiChunkBytes;
    m_ChunkInfo.m_uiUnreadChunkBytes = m_ChunkInfo.m_uiChunkBytes;

    m_ChunkInfo.m_bValid = true;

    return;
  }

  EZ_REPORT_FAILURE("Invalid chunk file, tag is '{0}'", szTag);
}

void ezChunkStreamReader::NextChunk()
{
  if (!m_ChunkInfo.m_bValid)
    return;

  const ezUInt64 uiToSkip = m_ChunkInfo.m_uiUnreadChunkBytes;
  const ezUInt64 uiSkipped = SkipBytes(uiToSkip);
  EZ_VERIFY(uiSkipped == uiToSkip, "Corrupt chunk '{0}' (version {1}), tried to skip {2} bytes, could only read {3} bytes", m_ChunkInfo.m_sChunkName, m_ChunkInfo.m_uiChunkVersion, uiToSkip, uiSkipped);

  TryReadChunkHeader();
}
