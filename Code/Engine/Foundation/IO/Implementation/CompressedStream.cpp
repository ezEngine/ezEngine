#include <Foundation/PCH.h>
#include <Foundation/Math/Math.h>
#include <Foundation/IO/CompressedStream.h>
#include <ThirdParty/zlib/zlib.h>

static voidpf zLibAlloc OF((voidpf opaque, uInt items, uInt size))
{
  return EZ_DEFAULT_NEW_RAW_BUFFER(ezUInt8, items * size);
}

static void zLibFree OF((voidpf opaque, voidpf address))
{
  ezUInt8* pData = (ezUInt8*) address;
  EZ_DEFAULT_DELETE_RAW_BUFFER(pData);
}

ezCompressedStreamReader::ezCompressedStreamReader(ezIBinaryStreamReader& InputStream) : m_InputStream(InputStream)
{
  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;
  m_pZLibStream = NULL;
}

ezCompressedStreamReader::~ezCompressedStreamReader()
{
  inflateEnd(m_pZLibStream);

  EZ_DEFAULT_DELETE(m_pZLibStream);
}

ezUInt64 ezCompressedStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  // if we have not read from the stream before, initialize everything
  if (m_pZLibStream == NULL)
  {
    // read how large the uncompressed data is
    m_InputStream.ReadBytes(&m_uiUncompressedSize, sizeof(ezUInt32));
    // read how large the compressed stream is
    m_InputStream.ReadBytes(&m_uiCompressedSize, sizeof(ezUInt32));

    m_pZLibStream = EZ_DEFAULT_NEW(z_stream_s);
    ezMemoryUtils::ZeroFill(m_pZLibStream);

    m_pZLibStream->opaque = NULL;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree = zLibFree;

    EZ_VERIFY(inflateInit(m_pZLibStream) == Z_OK, "Initializing the zlib stream for decompression failed.");
  }

  if (uiBytesToRead == 0)
    return 0;

  ezUInt64 uiBytesRead = 0;

  // Implement the 'skip n bytes' feature with a temp cache
  if (pReadBuffer == NULL)
  {
    ezUInt8 uiTemp[1024];

    while (uiBytesToRead > 0)
    {
      const ezUInt32 uiToRead = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToRead), 1024);

      uiBytesRead += ReadBytes(uiTemp, uiToRead);
      uiBytesToRead -= uiToRead;
    }

    return uiBytesRead;
  }


  m_pZLibStream->next_out  = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<ezUInt32>(uiBytesToRead);

  while (uiBytesToRead > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0)
    {
      // we know how large the compressed data is, so fill either the entire cache, or read the remaining compressed data
      const ezUInt32 uiToRead = ezMath::Min<ezUInt32>(1024, m_uiCompressedSize);
      m_uiCompressedSize -= uiToRead;

      EZ_VERIFY(m_InputStream.ReadBytes(&m_CompressedCache[0], uiToRead) == uiToRead, "Reading data failed.");

      m_pZLibStream->avail_in = uiToRead;
      m_pZLibStream->next_in = &m_CompressedCache[0];
    }

    m_pZLibStream->total_out = 0;

    const int iRet = inflate(m_pZLibStream, Z_NO_FLUSH);
    EZ_ASSERT(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '%s'", m_pZLibStream->msg);

    uiBytesToRead -= m_pZLibStream->total_out;
    uiBytesRead   += m_pZLibStream->total_out;

    if (iRet == Z_STREAM_END)
    {
      EZ_ASSERT(m_uiCompressedSize == 0, "Implementation error.");
      EZ_ASSERT(m_pZLibStream->avail_in == 0, "Implementation error.");
      return uiBytesRead;
    }
  }

  return uiBytesRead;
}


ezCompressedStreamWriter::ezCompressedStreamWriter(ezIBinaryStreamWriter& OutputStream, Compression Ratio) : m_OutputStream(OutputStream)
{
  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;

  m_pZLibStream = EZ_DEFAULT_NEW(z_stream_s);
  ezMemoryUtils::ZeroFill(m_pZLibStream);

  m_pZLibStream->opaque = NULL;
  m_pZLibStream->zalloc = zLibAlloc;
  m_pZLibStream->zfree = zLibFree;

  EZ_VERIFY(deflateInit(m_pZLibStream, Ratio) == Z_OK, "Initializing the zlib stream for compression failed.");
}

ezCompressedStreamWriter::~ezCompressedStreamWriter()
{
  CloseStream();
}

ezUInt32 ezCompressedStreamWriter::GetCompressedSize() const
{
  if (!m_CompressedData.IsEmpty())
    return m_CompressedData.GetCount();

  return m_uiCompressedSize;
}

void ezCompressedStreamWriter::CloseStream()
{
  if (m_pZLibStream == NULL)
    return;

  while (true)
  {
    ezUInt8 uiOutput[1024];

    m_pZLibStream->next_out  = uiOutput;
    m_pZLibStream->avail_out = 1024;
    m_pZLibStream->total_out = 0;

    const int iRes = deflate(m_pZLibStream, Z_FINISH);
    EZ_ASSERT(iRes == Z_STREAM_END || iRes == Z_OK, "Flushing the zlib compression stream did not finish the stream.");

    if (m_pZLibStream->total_out > 0)
      m_CompressedData.PushBackRange(ezArrayPtr<const ezUInt8>(uiOutput, m_pZLibStream->total_out));

    if (iRes == Z_STREAM_END)
      break;
  }

  // write how large the uncompressed data is
  m_OutputStream.WriteBytes(&m_uiUncompressedSize, sizeof(ezUInt32));

  // write how large the compressed stream is
  m_uiCompressedSize = m_CompressedData.GetCount();
  m_OutputStream.WriteBytes(&m_uiCompressedSize, sizeof(ezUInt32));

  // now write out the entire stream
  if (!m_CompressedData.IsEmpty())
    m_OutputStream.WriteBytes(&m_CompressedData[0], m_uiCompressedSize);

  EZ_VERIFY(deflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib compression stream failed.");

  m_CompressedData.Clear();

  EZ_DEFAULT_DELETE(m_pZLibStream);
}

ezResult ezCompressedStreamWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT(m_pZLibStream != NULL, "The stream has already been closed, you cannot write more data to it.");

  m_uiUncompressedSize += static_cast<ezUInt32>(uiBytesToWrite);

  ezUInt8 uiOutput[1024];

  m_pZLibStream->next_in  = static_cast<Bytef*>(const_cast<void*>(pWriteBuffer)); // C libraries suck at type safety
  m_pZLibStream->avail_in = static_cast<ezUInt32>(uiBytesToWrite);
  m_pZLibStream->total_in = 0;

  while (m_pZLibStream->avail_in > 0)
  {
    m_pZLibStream->next_out  = uiOutput;
    m_pZLibStream->avail_out = 1024;
    m_pZLibStream->total_out = 0;

    EZ_VERIFY(deflate(m_pZLibStream, Z_NO_FLUSH) == Z_OK, "Compressing the zlib stream failed.");

    if (m_pZLibStream->total_out > 0)
      m_CompressedData.PushBackRange(ezArrayPtr<const ezUInt8>(uiOutput, m_pZLibStream->total_out));
  }

  return EZ_SUCCESS;
}

