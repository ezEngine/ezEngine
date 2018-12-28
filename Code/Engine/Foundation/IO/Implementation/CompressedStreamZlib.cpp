#include <PCH.h>

#include <Foundation/IO/CompressedStreamZlib.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

#  include <ThirdParty/zlib/zlib.h>

static voidpf zLibAlloc OF((voidpf opaque, uInt items, uInt size))
{
  return EZ_DEFAULT_NEW_RAW_BUFFER(ezUInt8, items * size);
}

static void zLibFree OF((voidpf opaque, voidpf address))
{
  ezUInt8* pData = (ezUInt8*)address;
  EZ_DEFAULT_DELETE_RAW_BUFFER(pData);
}

EZ_DEFINE_AS_POD_TYPE(z_stream_s);

ezCompressedStreamReaderZlib::ezCompressedStreamReaderZlib(ezStreamReader* pInputStream)
    : m_pInputStream(pInputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);
}

ezCompressedStreamReaderZlib::~ezCompressedStreamReaderZlib()
{
  EZ_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);

  EZ_DEFAULT_DELETE(m_pZLibStream);
}

ezUInt64 ezCompressedStreamReaderZlib::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // if we have not read from the stream before, initialize everything
  if (m_pZLibStream == nullptr)
  {
    m_pZLibStream = EZ_DEFAULT_NEW(z_stream_s);
    EZ_ANALYSIS_ASSUME(m_pZLibStream != nullptr);
    ezMemoryUtils::ZeroFill(m_pZLibStream);

    m_pZLibStream->opaque = nullptr;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree = zLibFree;

    EZ_VERIFY(inflateInit(m_pZLibStream) == Z_OK, "Initializing the zlib stream for decompression failed: '{0}'", m_pZLibStream->msg);
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


  m_pZLibStream->next_out = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<ezUInt32>(uiBytesToRead);
  m_pZLibStream->total_out = 0;

  while (m_pZLibStream->avail_out > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0)
    {
      ezUInt16 uiCompressedSize = 0;
      EZ_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(ezUInt16)) == sizeof(ezUInt16),
                "Reading the compressed chunk size from the input stream failed.");

      m_pZLibStream->avail_in = uiCompressedSize;
      m_pZLibStream->next_in = m_CompressedCache.GetData();

      if (uiCompressedSize > 0)
      {
        EZ_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(ezUInt8) * uiCompressedSize) ==
                      sizeof(ezUInt8) * uiCompressedSize,
                  "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
      }
    }

    // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
    if (m_pZLibStream->avail_in == 0)
    {
      // in this case there is also no output that can be generated anymore
      m_bReachedEnd = true;
      return m_pZLibStream->total_out;
    }

    const int iRet = inflate(m_pZLibStream, Z_NO_FLUSH);
    EZ_ASSERT_DEV(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '{0}'", m_pZLibStream->msg);

    if (iRet == Z_STREAM_END)
    {
      m_bReachedEnd = true;

      // if we have reached the end, we have not yet read the zero-terminator
      // do this now, so that data that comes after the compressed stream can be read properly

      ezUInt16 uiTerminator = 0;
      EZ_VERIFY(m_pInputStream->ReadBytes(&uiTerminator, sizeof(ezUInt16)) == sizeof(ezUInt16),
                "Reading the compressed stream terminator failed.");

      EZ_ASSERT_DEV(uiTerminator == 0, "Unexpected Stream Terminator: {0}", uiTerminator);
      EZ_ASSERT_DEV(m_pZLibStream->avail_in == 0, "The input buffer should be depleted, but {0} bytes are still there.",
                    m_pZLibStream->avail_in);
      return m_pZLibStream->total_out;
    }
  }

  return m_pZLibStream->total_out;
}


ezCompressedStreamWriterZlib::ezCompressedStreamWriterZlib(ezStreamWriter* pOutputStream, Compression Ratio)
    : m_pOutputStream(pOutputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);

  m_pZLibStream = EZ_DEFAULT_NEW(z_stream_s);
  EZ_ANALYSIS_ASSUME(m_pZLibStream != nullptr);

  ezMemoryUtils::ZeroFill(m_pZLibStream);

  m_pZLibStream->opaque = nullptr;
  m_pZLibStream->zalloc = zLibAlloc;
  m_pZLibStream->zfree = zLibFree;
  m_pZLibStream->next_out = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();
  m_pZLibStream->total_out = 0;

  EZ_VERIFY(deflateInit(m_pZLibStream, Ratio) == Z_OK, "Initializing the zlib stream for compression failed: '{0}'", m_pZLibStream->msg);
}

ezCompressedStreamWriterZlib::~ezCompressedStreamWriterZlib()
{
  CloseStream();
}

ezResult ezCompressedStreamWriterZlib::CloseStream()
{
  if (m_pZLibStream == nullptr)
    return EZ_SUCCESS;

  ezInt32 iRes = Z_OK;
  while (iRes == Z_OK)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == EZ_FAILURE)
        return EZ_FAILURE;
    }

    iRes = deflate(m_pZLibStream, Z_FINISH);
    EZ_ASSERT_DEV(iRes == Z_STREAM_END || iRes == Z_OK, "Finishing the stream failed: '{0}'", m_pZLibStream->msg);
  }

  // one more flush to write out the last chunk
  if (Flush() == EZ_FAILURE)
    return EZ_FAILURE;

  // write a zero-terminator
  const ezUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(ezUInt16)) == EZ_FAILURE)
    return EZ_FAILURE;

  EZ_VERIFY(deflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib compression stream failed: '{0}'", m_pZLibStream->msg);
  EZ_DEFAULT_DELETE(m_pZLibStream);

  return EZ_SUCCESS;
}

ezResult ezCompressedStreamWriterZlib::Flush()
{
  if (m_pZLibStream == nullptr)
    return EZ_SUCCESS;

  const ezUInt16 uiUsedCache = static_cast<ezUInt16>(m_pZLibStream->total_out);

  if (uiUsedCache == 0)
    return EZ_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(ezUInt16)) == EZ_FAILURE)
    return EZ_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(ezUInt8) * uiUsedCache) == EZ_FAILURE)
    return EZ_FAILURE;

  m_uiCompressedSize += uiUsedCache;

  m_pZLibStream->total_out = 0;
  m_pZLibStream->next_out = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();

  return EZ_SUCCESS;
}

ezResult ezCompressedStreamWriterZlib::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_pZLibStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += uiBytesToWrite;

  m_pZLibStream->next_in = static_cast<Bytef*>(const_cast<void*>(pWriteBuffer)); // C libraries suck at type safety
  m_pZLibStream->avail_in = static_cast<ezUInt32>(uiBytesToWrite);
  m_pZLibStream->total_in = 0;

  while (m_pZLibStream->avail_in > 0)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == EZ_FAILURE)
        return EZ_FAILURE;
    }

    EZ_VERIFY(deflate(m_pZLibStream, Z_NO_FLUSH) == Z_OK, "Compressing the zlib stream failed: '{0}'", m_pZLibStream->msg);
  }

  return EZ_SUCCESS;
}

#endif // BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_CompressedStream);
