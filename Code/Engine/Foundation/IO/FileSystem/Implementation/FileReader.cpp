#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>

ezResult ezFileReader::Open(ezStringView sFile, ezUInt32 uiCacheSize /*= 1024 * 64*/,
  ezFileShareMode::Enum fileShareMode /*= ezFileShareMode::SharedReads*/, bool bAllowFileEvents /*= true*/)
{
  EZ_ASSERT_DEV(m_pDataDirReader == nullptr, "The file reader is already open. (File: '{0}')", sFile);

  uiCacheSize = ezMath::Min<ezUInt32>(uiCacheSize, 1024 * 1024 * 32);

  m_pDataDirReader = GetFileReader(sFile, fileShareMode, bAllowFileEvents);

  if (!m_pDataDirReader)
    return EZ_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheReadPosition = 0;
  m_uiBytesCached = 0;
  m_bEOF = false;

  return EZ_SUCCESS;
}

void ezFileReader::Close()
{
  if (m_pDataDirReader)
    m_pDataDirReader->Close();

  m_pDataDirReader = nullptr;
  m_bEOF = true;
}

ezUInt64 ezFileReader::SkipBytes(ezUInt64 uiBytesToSkip)
{
  EZ_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  ezUInt64 uiSkipPosition = 0; // how much was skipped, yet

  // if any data is still in the cache, skip that first
  {
    const ezUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
    const ezUInt64 uiBytesSkippedFromCache = ezMath::Min(uiCachedBytesLeft, uiBytesToSkip);
    uiSkipPosition += uiBytesSkippedFromCache;
    m_uiCacheReadPosition += uiBytesSkippedFromCache;
    uiBytesToSkip -= uiBytesSkippedFromCache;
  }

  // skip bytes on disk
  const ezUInt64 uiBytesMeantToSkipFromDisk = uiBytesToSkip;
  const ezUInt64 uiBytesSkippedFromDisk = m_pDataDirReader->Skip(uiBytesToSkip);
  uiSkipPosition += uiBytesSkippedFromDisk;
  uiBytesToSkip -= uiBytesSkippedFromDisk;

  // mark end of file if suitable
  const bool endOfCacheReached = m_uiCacheReadPosition == m_uiBytesCached;
  const bool endOfDiskDataReached = uiBytesSkippedFromDisk < uiBytesMeantToSkipFromDisk;
  if (endOfCacheReached && endOfDiskDataReached)
  {
    m_bEOF = true;
  }

  return uiSkipPosition;
}

ezUInt64 ezFileReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  EZ_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  ezUInt64 uiBufferPosition = 0; // how much was read, yet
  ezUInt8* pBuffer = (ezUInt8*)pReadBuffer;

  if (uiBytesToRead > m_Cache.GetCount())
  {
    // if any data is still in the cache, use that first
    const ezUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
    if (uiCachedBytesLeft > 0)
    {
      ezMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(ezUInt32)m_uiCacheReadPosition], (ezUInt32)uiCachedBytesLeft);
      uiBufferPosition += uiCachedBytesLeft;
      m_uiCacheReadPosition += uiCachedBytesLeft;
      uiBytesToRead -= uiCachedBytesLeft;
    }

    // read remaining data from disk
    ezUInt64 uiBytesReadFromDisk = 0;
    if (uiBytesToRead > 0)
    {
      uiBytesReadFromDisk = m_pDataDirReader->Read(&pBuffer[uiBufferPosition], uiBytesToRead);
      uiBufferPosition += uiBytesReadFromDisk;
    }

    // mark eof if we're already there
    if (uiBytesReadFromDisk == 0)
    {
      m_bEOF = true;
      return uiBufferPosition;
    }
  }
  else
  {
    while (uiBytesToRead > 0)
    {
      // determine the chunk size to read
      ezUInt64 uiChunkSize = uiBytesToRead;

      const ezUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
      if (uiCachedBytesLeft < uiBytesToRead)
      {
        uiChunkSize = uiCachedBytesLeft;
      }

      // copy data into the buffer
      // uiChunkSize can never be larger than the cache size, which is limited to 32 Bit
      if (uiChunkSize > 0)
      {
        ezMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(ezUInt32)m_uiCacheReadPosition], (ezUInt32)uiChunkSize);

        // store how much was read and how much is still left to read
        uiBufferPosition += uiChunkSize;
        m_uiCacheReadPosition += uiChunkSize;
        uiBytesToRead -= uiChunkSize;
      }

      // if the cache is depleted, refill it
      // this will even be triggered if EXACTLY the amount of available bytes was read
      if (m_uiCacheReadPosition >= m_uiBytesCached)
      {
        m_uiBytesCached = m_pDataDirReader->Read(&m_Cache[0], m_Cache.GetCount());
        m_uiCacheReadPosition = 0;

        // if nothing else could be read from the file, return the number of bytes that have been read
        if (m_uiBytesCached == 0)
        {
          // if absolutely nothing could be read, we reached the end of the file (and we actually returned everything else,
          // so the file was really read to the end).
          m_bEOF = true;
          return uiBufferPosition;
        }
      }
    }
  }

  // return how much was read
  return uiBufferPosition;
}
