#include <Foundation/PCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Math.h>

ezResult ezFileWriter::Open(const char* szFile, ezUInt32 uiCacheSize, bool bAllowFileEvents)
{
  uiCacheSize = ezMath::Clamp<ezUInt32>(uiCacheSize, 1024, 1024 * 1024 * 32);

  m_pDataDirWriter = GetFileWriter(szFile, bAllowFileEvents);

  if (!m_pDataDirWriter)
    return EZ_FAILURE;

  m_Cache.SetCount(uiCacheSize);

  m_uiCacheWritePosition = 0;

  return EZ_SUCCESS;
}

void ezFileWriter::Close()
{
  if (!m_pDataDirWriter)
    return;

  Flush();
  
  m_pDataDirWriter->Close();
  m_pDataDirWriter = nullptr;
}

ezResult ezFileWriter::Flush()
{
  const ezResult res = m_pDataDirWriter->Write(&m_Cache[0], m_uiCacheWritePosition);
  m_uiCacheWritePosition = 0;

  return res;
}

ezResult ezFileWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_pDataDirWriter != nullptr, "The file has not been opened (successfully).");

  ezUInt8* pBuffer = (ezUInt8*) pWriteBuffer;

  while (uiBytesToWrite > 0)
  {
    // determine chunk size to be written
    ezUInt64 uiChunkSize = uiBytesToWrite;

    const ezUInt64 uiRemainingCache = m_Cache.GetCount() - m_uiCacheWritePosition;

    if (uiRemainingCache < uiBytesToWrite)
      uiChunkSize = uiRemainingCache;

    // copy memory
    ezMemoryUtils::Copy(&m_Cache[(ezUInt32) m_uiCacheWritePosition], pBuffer, (ezUInt32) uiChunkSize);

    pBuffer += uiChunkSize;
    m_uiCacheWritePosition += uiChunkSize;
    uiBytesToWrite -= uiChunkSize;

    // if the cache is full or nearly full, flush it to disk
    if (m_uiCacheWritePosition + 32 >= m_Cache.GetCount())
    {
      if (Flush() == EZ_FAILURE)
        return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileWriter);

