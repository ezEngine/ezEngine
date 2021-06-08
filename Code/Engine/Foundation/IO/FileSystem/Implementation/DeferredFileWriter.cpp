#include <FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezDeferredFileWriter::ezDeferredFileWriter()
  : m_Writer(&m_Storage)
{
}

void ezDeferredFileWriter::SetOutput(const char* szFileToWriteTo, bool bOnlyWriteIfDifferent)
{
  m_bOnlyWriteIfDifferent = bOnlyWriteIfDifferent;
  m_sOutputFile = szFileToWriteTo;
}

ezResult ezDeferredFileWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

ezResult ezDeferredFileWriter::Close()
{
  if (m_bAlreadyClosed)
    return EZ_SUCCESS;

  if (m_sOutputFile.IsEmpty())
    return EZ_FAILURE;

  m_bAlreadyClosed = true;

  if (m_bOnlyWriteIfDifferent)
  {
    ezFileReader fileIn;
    if (fileIn.Open(m_sOutputFile).Succeeded() && fileIn.GetFileSize() == m_Storage.GetStorageSize())
    {
      ezUInt8 tmp[1024 * 8];

      const ezUInt8* pData = m_Storage.GetData();
      ezUInt64 readLeft = m_Storage.GetStorageSize();

      while (readLeft > 0)
      {
        const ezUInt64 toRead = ezMath::Min<ezUInt64>(readLeft, EZ_ARRAY_SIZE(tmp));
        const ezUInt64 readBytes = fileIn.ReadBytes(tmp, toRead);

        if (toRead != readBytes)
          return EZ_FAILURE;

        readLeft -= toRead;

        if (ezMemoryUtils::RawByteCompare(tmp, pData, ezMath::SafeConvertToSizeT(readBytes)) != 0)
          goto write_data;

        pData += readBytes;
      }

      // content is already the same as what we would write -> skip the write (do not modify file write date)
      return EZ_SUCCESS;
    }
  }

write_data:
  ezFileWriter file;
  if (file.Open(m_sOutputFile).Failed())
    return EZ_FAILURE;

  m_sOutputFile.Clear();
  return file.WriteBytes(m_Storage.GetData(), m_Storage.GetStorageSize());
}

void ezDeferredFileWriter::Discard()
{
  m_sOutputFile.Clear();
}

EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DeferredFileWriter);
