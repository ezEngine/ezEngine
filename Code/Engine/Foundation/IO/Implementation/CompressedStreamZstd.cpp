#include <PCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

#  include <ThirdParty/zstd/zstd.h>

ezCompressedStreamReaderZstd::ezCompressedStreamReaderZstd(ezStreamReader* pInputStream)
    : m_pInputStream(pInputStream)
{
  m_InBuffer.pos = 0;
  m_InBuffer.size = 0;
}

ezCompressedStreamReaderZstd::~ezCompressedStreamReaderZstd()
{
  if (m_pZstdDStream != nullptr)
  {
    ZSTD_freeDStream(m_pZstdDStream);
    m_pZstdDStream = nullptr;
  }
}

ezUInt64 ezCompressedStreamReaderZstd::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // if we have not read from the stream before, initialize everything
  if (m_pZstdDStream == nullptr)
  {
    m_pZstdDStream = ZSTD_createDStream();

    ZSTD_initDStream(m_pZstdDStream);
  }

  // Implement the 'skip n bytes' feature with a temp cache
  if (pReadBuffer == nullptr)
  {
    ezUInt64 uiBytesRead = 0;
    ezUInt8 uiTemp[1024];

    while (uiBytesToRead > 0)
    {
      const ezUInt32 uiToRead = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToRead), 1024);

      const ezUInt64 uiGotBytes = ReadBytes(uiTemp, uiToRead);

      uiBytesRead += uiGotBytes;
      uiBytesToRead -= uiGotBytes;

      if (uiGotBytes == 0) // prevent an endless loop
        break;
    }

    return uiBytesRead;
  }

  ZSTD_outBuffer outBuffer;
  outBuffer.dst = pReadBuffer;
  outBuffer.pos = 0;
  outBuffer.size = uiBytesToRead;

  while (outBuffer.pos < outBuffer.size)
  {
    if (RefillReadCache().Failed())
      return outBuffer.pos;

    const size_t res = ZSTD_decompressStream(m_pZstdDStream, &outBuffer, &m_InBuffer);
    EZ_ASSERT_DEV(!ZSTD_isError(res), "Decompressing the stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  if (m_InBuffer.pos == m_InBuffer.size)
  {
    // if we have reached the end, we have not yet read the zero-terminator
    // do this now, so that data that comes after the compressed stream can be read properly

    RefillReadCache();
  }

  return outBuffer.pos;
}

ezResult ezCompressedStreamReaderZstd::RefillReadCache()
{
  // if our input buffer is empty, we need to read more into our cache
  if (m_InBuffer.pos == m_InBuffer.size)
  {
    ezUInt16 uiCompressedSize = 0;
    EZ_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(ezUInt16)) == sizeof(ezUInt16),
              "Reading the compressed chunk size from the input stream failed.");

    m_InBuffer.pos = 0;
    m_InBuffer.size = uiCompressedSize;

    if (uiCompressedSize > 0)
    {
      if (m_CompressedCache.GetCount() < uiCompressedSize)
      {
        m_CompressedCache.SetCountUninitialized(ezMath::RoundUp(uiCompressedSize, 1024));

        m_InBuffer.src = m_CompressedCache.GetData();
      }

      EZ_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(ezUInt8) * uiCompressedSize) ==
                    sizeof(ezUInt8) * uiCompressedSize,
                "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
    }
  }

  // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
  if (m_InBuffer.size == 0)
  {
    // in this case there is also no output that can be generated anymore
    m_bReachedEnd = true;
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezCompressedStreamWriterZstd::ezCompressedStreamWriterZstd() = default;

ezCompressedStreamWriterZstd::ezCompressedStreamWriterZstd(ezStreamWriter* pOutputStream, Compression Ratio)
{
  SetOutputStream(pOutputStream, Ratio);
}

ezCompressedStreamWriterZstd::~ezCompressedStreamWriterZstd()
{
  FinishCompressedStream();

  if (m_pZstdCStream)
  {
    ZSTD_freeCStream(m_pZstdCStream);
    m_pZstdCStream = nullptr;
  }
}

void ezCompressedStreamWriterZstd::SetOutputStream(ezStreamWriter* pOutputStream, Compression Ratio /*= Compression::Default*/,
                                                   ezUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  if (m_pOutputStream == pOutputStream)
    return;

  // finish anything done on a previous output stream
  FinishCompressedStream();

  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;

  if (pOutputStream != nullptr)
  {
    m_pOutputStream = pOutputStream;

    if (m_pZstdCStream == nullptr)
    {
      m_pZstdCStream = ZSTD_createCStream();
    }

    ZSTD_initCStream(m_pZstdCStream, (int)Ratio);

    m_CompressedCache.SetCountUninitialized(ezMath::Max(1U, uiCompressionCacheSizeKB) * 1024);

    m_OutBuffer.dst = m_CompressedCache.GetData();
    m_OutBuffer.pos = 0;
    m_OutBuffer.size = m_CompressedCache.GetCount();
  }
}

ezResult ezCompressedStreamWriterZstd::FinishCompressedStream()
{
  if (m_pOutputStream == nullptr)
    return EZ_SUCCESS;

  if (Flush().Failed())
    return EZ_FAILURE;

  const size_t res = ZSTD_endStream(m_pZstdCStream, &m_OutBuffer);
  EZ_VERIFY(!ZSTD_isError(res), "Deinitializing the zstd compression stream failed: '{0}'", ZSTD_getErrorName(res));

  // one more flush to write out the last chunk
  if (FlushWriteCache() == EZ_FAILURE)
    return EZ_FAILURE;

  // write a zero-terminator
  const ezUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(ezUInt16)) == EZ_FAILURE)
    return EZ_FAILURE;

  m_pOutputStream = nullptr;

  return EZ_SUCCESS;
}

ezResult ezCompressedStreamWriterZstd::Flush()
{
  if (m_pOutputStream == nullptr)
    return EZ_SUCCESS;

  while (ZSTD_flushStream(m_pZstdCStream, &m_OutBuffer) > 0)
  {
    if (FlushWriteCache() == EZ_FAILURE)
      return EZ_FAILURE;
  }

  if (FlushWriteCache() == EZ_FAILURE)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezCompressedStreamWriterZstd::FlushWriteCache()
{
  if (m_pOutputStream == nullptr)
    return EZ_SUCCESS;

  const ezUInt16 uiUsedCache = static_cast<ezUInt16>(m_OutBuffer.pos);

  if (uiUsedCache == 0)
    return EZ_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(ezUInt16)) == EZ_FAILURE)
    return EZ_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(ezUInt8) * uiUsedCache) == EZ_FAILURE)
    return EZ_FAILURE;

  m_uiCompressedSize += uiUsedCache;

  // reset the write position
  m_OutBuffer.pos = 0;

  return EZ_SUCCESS;
}

ezResult ezCompressedStreamWriterZstd::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_pZstdCStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += static_cast<ezUInt32>(uiBytesToWrite);

  ZSTD_inBuffer inBuffer;
  inBuffer.pos = 0;
  inBuffer.src = pWriteBuffer;
  inBuffer.size = static_cast<size_t>(uiBytesToWrite);

  while (inBuffer.pos < inBuffer.size)
  {
    if (m_OutBuffer.pos == m_OutBuffer.size)
    {
      if (FlushWriteCache() == EZ_FAILURE)
        return EZ_FAILURE;
    }

    const size_t res = ZSTD_compressStream(m_pZstdCStream, &m_OutBuffer, &inBuffer);

    EZ_VERIFY(!ZSTD_isError(res), "Compressing the zstd stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  return EZ_SUCCESS;
}

#endif
