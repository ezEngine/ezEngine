#pragma once

#include <Foundation/IO/OSFile.h>

class StreamComparer : public ezStreamWriterBase
{
public:
  StreamComparer(const char* szExpectedData, bool bOnlyWriteResult = false)
  {
    m_bOnlyWriteResult = bOnlyWriteResult;
    m_szExpectedData = szExpectedData;
  }

  ~StreamComparer()
  {
    if (m_bOnlyWriteResult)
    {
      ezOSFile f;
      f.Open("C:\\Code\\JSON.txt", ezFileMode::Write);
      f.Write(m_sResult.GetData(), m_sResult.GetElementCount());
      f.Close();
    }
    else
      EZ_TEST_BOOL(*m_szExpectedData == '\0');
  }

  ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
  {
    if (m_bOnlyWriteResult)
      m_sResult.Append((const char*) pWriteBuffer);
    else
    {
      EZ_TEST_BOOL(ezMemoryUtils::IsEqual((const char*) pWriteBuffer, m_szExpectedData, (ezUInt32) uiBytesToWrite));
      m_szExpectedData += uiBytesToWrite;
    }

    return EZ_SUCCESS;
  }

private:
  bool m_bOnlyWriteResult;
  ezStringBuilder m_sResult;
  const char* m_szExpectedData;
};


class StringStream : public ezStreamReaderBase
{
public:

  StringStream(const void* pData)
  {
    m_pData = pData;
    m_uiLength = ezStringUtils::GetStringElementCount((const char*) pData);
  }

  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
  {
    uiBytesToRead = ezMath::Min(uiBytesToRead, m_uiLength);
    m_uiLength -= uiBytesToRead;

    if (uiBytesToRead > 0)
    {
      ezMemoryUtils::Copy((ezUInt8*) pReadBuffer, (ezUInt8*) m_pData, (size_t) uiBytesToRead);
      m_pData = ezMemoryUtils::AddByteOffsetConst(m_pData, (ptrdiff_t) uiBytesToRead);
    }

    return uiBytesToRead;
  }

private:
  const void* m_pData;
  ezUInt64 m_uiLength;
};


