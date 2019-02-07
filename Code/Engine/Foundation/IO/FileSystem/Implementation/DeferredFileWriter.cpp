#include <FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

ezDeferredFileWriter::ezDeferredFileWriter() : m_Writer(&m_Storage)
{
}

void ezDeferredFileWriter::SetOutput(const char* szFileToWriteTo)
{
  m_sOutputFile = szFileToWriteTo;
}

ezResult ezDeferredFileWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

ezResult ezDeferredFileWriter::Close()
{
  if (m_sOutputFile.IsEmpty())
    return EZ_FAILURE;

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

