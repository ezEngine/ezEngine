#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezDeferredFileWriter::ezDeferredFileWriter()
  : m_Writer(&m_Storage)
{
}

void ezDeferredFileWriter::SetOutput(ezStringView sFileToWriteTo, bool bOnlyWriteIfDifferent)
{
  m_bOnlyWriteIfDifferent = bOnlyWriteIfDifferent;
  m_sOutputFile = sFileToWriteTo;
}

ezResult ezDeferredFileWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

ezResult ezDeferredFileWriter::Close(bool* out_pWasWrittenTo /*= nullptr*/)
{
  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = false;
  }

  if (m_bAlreadyClosed)
    return EZ_SUCCESS;

  if (m_sOutputFile.IsEmpty())
    return EZ_FAILURE;

  m_bAlreadyClosed = true;

  if (m_bOnlyWriteIfDifferent)
  {
    ezFileReader fileIn;
    if (fileIn.Open(m_sOutputFile).Succeeded() && fileIn.GetFileSize() == m_Storage.GetStorageSize64())
    {
      ezUInt8 tmp1[1024 * 4];
      ezUInt8 tmp2[1024 * 4];

      ezMemoryStreamReader storageReader(&m_Storage);

      while (true)
      {
        const ezUInt64 readBytes1 = fileIn.ReadBytes(tmp1, EZ_ARRAY_SIZE(tmp1));
        const ezUInt64 readBytes2 = storageReader.ReadBytes(tmp2, EZ_ARRAY_SIZE(tmp2));

        if (readBytes1 != readBytes2)
          goto write_data;

        if (readBytes1 == 0)
          break;

        if (ezMemoryUtils::RawByteCompare(tmp1, tmp2, ezMath::SafeConvertToSizeT(readBytes1)) != 0)
          goto write_data;
      }

      // content is already the same as what we would write -> skip the write (do not modify file write date)
      return EZ_SUCCESS;
    }
  }

write_data:
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(m_sOutputFile, 0)); // use the minimum cache size, we want to pass data directly through to disk

  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = true;
  }

  m_sOutputFile.Clear();
  return m_Storage.CopyToStream(file);
}

void ezDeferredFileWriter::Discard()
{
  m_sOutputFile.Clear();
}


