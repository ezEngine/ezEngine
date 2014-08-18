#include <CoreUtils/PCH.h>
#include <CoreUtils/DataStructures/ChunkFormat.h>

ezChunkFormatWriter::ezChunkFormatWriter(ezStreamWriterBase* pStream)
{
  EZ_ASSERT(pStream != nullptr, "The passed in stream may not be NULL.");
  m_pStream = pStream;
  m_bWritingFile = false;
  m_bWritingChunk = false;
}

void ezChunkFormatWriter::BeginChunkFile()
{
  EZ_ASSERT(!m_bWritingFile, "Already writing the file.");

  m_bWritingFile = true;

  const char* szTag = "BGN CHNK";
  m_pStream->WriteBytes(szTag, 8);
}

void ezChunkFormatWriter::EndChunkFile()
{
  EZ_ASSERT(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT(!m_bWritingChunk, "A chunk is still open for writing: '%s'", m_sChunkName.GetData());

  m_bWritingFile = false;

  const char* szTag = "END CHNK";
  m_pStream->WriteBytes(szTag, 8);
}

void ezChunkFormatWriter::BeginChunk(const char* szName, ezUInt32 uiVersion)
{
  EZ_ASSERT(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT(!m_bWritingChunk, "A chunk is already open for writing: '%s'", m_sChunkName.GetData());

  m_sChunkName = szName;

  const char* szTag = "NXT CHNK";
  m_pStream->WriteBytes(szTag, 8);

  *m_pStream << m_sChunkName;
  *m_pStream << uiVersion;

  m_bWritingChunk = true;
}


void ezChunkFormatWriter::EndChunk()
{
  EZ_ASSERT(m_bWritingFile, "Not writing to the file.");
  EZ_ASSERT(m_bWritingChunk, "No chunk is currently open.");

  m_bWritingChunk = false;

  *m_pStream << m_Storage.GetCount();

  for (ezUInt32 i = 0; i < m_Storage.GetCount(); )
  {
    const ezUInt32 uiRange = m_Storage.GetContiguousRange(i);

    EZ_ASSERT(uiRange > 0, "Invalid contiguous range");

    m_pStream->WriteBytes(&m_Storage[i], uiRange);
    i += uiRange;
  }

  m_Storage.Clear();
}

ezResult ezChunkFormatWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT(m_bWritingChunk, "No chunk is currently written to");

  const ezUInt8* pBytes = (const ezUInt8*) pWriteBuffer;

  for (ezUInt64 i = 0; i < uiBytesToWrite; ++i)
    m_Storage.PushBack(pBytes[i]);

  return EZ_SUCCESS;
}





ezChunkFormatReader::ezChunkFormatReader(ezStreamReaderBase* pStream)
{
  EZ_ASSERT(pStream != nullptr, "The passed in stream may not be NULL.");
  m_pStream = pStream;

  m_ChunkInfo.m_bValid = false;
}

ezUInt64 ezChunkFormatReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  EZ_ASSERT(m_ChunkInfo.m_bValid, "No valid chunk available.");

  uiBytesToRead = ezMath::Min<ezUInt64>(uiBytesToRead, m_ChunkInfo.m_uiUnreadChunkBytes);
  m_ChunkInfo.m_uiUnreadChunkBytes -= (ezUInt32) uiBytesToRead;

  return m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
}

void ezChunkFormatReader::BeginChunkFile()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_pStream->ReadBytes(szTag, 8);
  szTag[8] = '\0';

  EZ_ASSERT(ezStringUtils::IsEqual(szTag, "BGN CHNK"), "Not a valid chunk file.");

  TryReadChunkHeader();
}

void ezChunkFormatReader::EndChunkFile(EndChunkFileMode mode)
{
  if (mode == EndChunkFileMode::SkipToEnd)
  {
    while (m_ChunkInfo.m_bValid)
      NextChunk();
  }
}

void ezChunkFormatReader::TryReadChunkHeader()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_pStream->ReadBytes(szTag, 8);
  szTag[8] = '\0';

  if (ezStringUtils::IsEqual(szTag, "END CHNK"))
    return;
  
  if (ezStringUtils::IsEqual(szTag, "NXT CHNK"))
  {
    *m_pStream >> m_ChunkInfo.m_sChunkName;
    *m_pStream >> m_ChunkInfo.m_uiChunkVersion;
    *m_pStream >> m_ChunkInfo.m_uiChunkBytes;
    m_ChunkInfo.m_uiUnreadChunkBytes = m_ChunkInfo.m_uiChunkBytes;

    m_ChunkInfo.m_bValid = true;

    return;
  }

  EZ_REPORT_FAILURE("Invalid chunk file, tag is '%s'", szTag);
}

void ezChunkFormatReader::NextChunk()
{
  if (!m_ChunkInfo.m_bValid)
    return;

  const ezUInt64 uiToSkip  = m_ChunkInfo.m_uiUnreadChunkBytes;
  const ezUInt64 uiSkipped = SkipBytes(uiToSkip);
  EZ_VERIFY(uiSkipped == uiToSkip, "Corrupt chunk '%s' (version %u), tried to skip %llu bytes, could only read %llu bytes", m_ChunkInfo.m_sChunkName.GetData(), m_ChunkInfo.m_uiChunkVersion, uiToSkip, uiSkipped);

  TryReadChunkHeader();
}


