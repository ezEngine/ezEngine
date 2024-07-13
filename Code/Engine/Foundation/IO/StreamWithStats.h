
#pragma once

#include <Foundation/IO/Stream.h>

/// \brief A stream reader that wraps another stream to track how many bytes are read from it.
class EZ_FOUNDATION_DLL ezStreamReaderWithStats : public ezStreamReader
{
public:
  ezStreamReaderWithStats() = default;
  ezStreamReaderWithStats(ezStreamReader* stream)
    : m_pStream(stream)
  {
  }

  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override
  {
    const ezUInt64 uiRead = m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
    m_uiBytesRead += uiRead;
    return uiRead;
  }

  ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip) override
  {
    const ezUInt64 uiSkipped = m_pStream->SkipBytes(uiBytesToSkip);
    m_uiBytesSkipped += uiSkipped;
    return uiSkipped;
  }

  /// the stream to forward all requests to
  ezStreamReader* m_pStream = nullptr;

  /// the number of bytes that were read from the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  ezUInt64 m_uiBytesRead = 0;

  /// the number of bytes that were skipped from the wrapped stream
  ezUInt64 m_uiBytesSkipped = 0;
};

/// \brief A stream writer that wraps another stream to track how many bytes are written to it.
class EZ_FOUNDATION_DLL ezStreamWriterWithStats : public ezStreamWriter
{
public:
  ezStreamWriterWithStats() = default;
  ezStreamWriterWithStats(ezStreamWriter* stream)
    : m_pStream(stream)
  {
  }

  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override
  {
    m_uiBytesWritten += uiBytesToWrite;
    return m_pStream->WriteBytes(pWriteBuffer, uiBytesToWrite);
  }

  ezResult Flush() override
  {
    return m_pStream->Flush();
  }

  /// the stream to forward all requests to
  ezStreamWriter* m_pStream = nullptr;

  /// the number of bytes that were written to the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  ezUInt64 m_uiBytesWritten = 0;
};
