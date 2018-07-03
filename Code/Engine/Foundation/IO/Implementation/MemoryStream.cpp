#include <PCH.h>

#include <Foundation/IO/MemoryStream.h>

// Reader implementation

ezMemoryStreamReader::ezMemoryStreamReader(ezMemoryStreamStorageInterface* pStreamStorage)
    : m_pStreamStorage(pStreamStorage), m_uiReadPosition(0)
{
}

ezMemoryStreamReader::~ezMemoryStreamReader()
{
}

ezUInt64 ezMemoryStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const ezUInt32 uiBytes = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToRead), m_pStreamStorage->GetStorageSize() - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
    ezMemoryUtils::Copy(static_cast<ezUInt8*>(pReadBuffer), &m_pStreamStorage->GetInternalData()[m_uiReadPosition], uiBytes);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

ezUInt64 ezMemoryStreamReader::SkipBytes(ezUInt64 uiBytesToSkip)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const ezUInt32 uiBytes = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToSkip), m_pStreamStorage->GetStorageSize() - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void ezMemoryStreamReader::SetReadPosition(ezUInt32 uiReadPosition)
{
  EZ_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

ezUInt32 ezMemoryStreamReader::GetByteCount() const
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize();
}


void ezMemoryStreamReader::SetDebugSourceInformation(const char* szDebugSourceInformation)
{
  m_DebugSourceInformation = szDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////

// Writer implementation
ezMemoryStreamWriter::ezMemoryStreamWriter(ezMemoryStreamStorageInterface* pStreamStorage)
    : m_pStreamStorage(pStreamStorage), m_uiWritePosition(0)
{
}

ezMemoryStreamWriter::~ezMemoryStreamWriter()
{
}

ezResult ezMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  if (uiBytesToWrite == 0)
    return EZ_SUCCESS;

  EZ_ASSERT_DEBUG(pWriteBuffer != nullptr, "No valid buffer containing data given!");

  ezUInt64 uiNewSizeInBytes = m_uiWritePosition + uiBytesToWrite;
  EZ_IGNORE_UNUSED(uiNewSizeInBytes);
  EZ_ASSERT_DEV(uiNewSizeInBytes < ezInvalidIndex, "Memory stream only supports up to 4GB of data");

  const ezUInt32 uiBytesToWrite32 = static_cast<ezUInt32>(uiBytesToWrite);

  // Reserve the memory in the storage object
  m_pStreamStorage->SetInternalSize(uiBytesToWrite32 + m_uiWritePosition);

  ezUInt8* pWritePointer = &m_pStreamStorage->GetInternalData()[m_uiWritePosition];

  ezMemoryUtils::Copy(pWritePointer, static_cast<const ezUInt8*>(pWriteBuffer), uiBytesToWrite32);

  m_uiWritePosition += uiBytesToWrite32;

  return EZ_SUCCESS;
}

void ezMemoryStreamWriter::SetWritePosition(ezUInt32 uiWritePosition)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  EZ_ASSERT_RELEASE(uiWritePosition <= GetByteCount(), "Write position must be between 0 and GetByteCount()!");
  m_uiWritePosition = uiWritePosition;
}

ezUInt32 ezMemoryStreamWriter::GetByteCount() const
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize();
}

//////////////////////////////////////////////////////////////////////////

void ezMemoryStreamStorageInterface::ReadAll(ezStreamReader& Stream)
{
  Clear();
  ezMemoryStreamWriter w(this);

  ezUInt8 uiTemp[1024];

  while (true)
  {
    const ezUInt64 uiRead = Stream.ReadBytes(uiTemp, 1024);
    w.WriteBytes(uiTemp, uiRead);
    if (uiRead < 1024)
      break;
  }
}

//////////////////////////////////////////////////////////////////////////


ezRawMemoryStreamReader::ezRawMemoryStreamReader(const void* pData, ezUInt32 uiDataSize)
{
  EZ_ASSERT_DEV(pData != nullptr, "Invalid data storage");

  m_pRawMemory = static_cast<const ezUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiReadPosition = 0;
}

ezRawMemoryStreamReader::~ezRawMemoryStreamReader()
{
}

ezUInt64 ezRawMemoryStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  const ezUInt32 uiBytes = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToRead), m_uiChunkSize - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    ezMemoryUtils::Copy(static_cast<ezUInt8*>(pReadBuffer), &m_pRawMemory[m_uiReadPosition], uiBytes);
  }

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

ezUInt64 ezRawMemoryStreamReader::SkipBytes(ezUInt64 uiBytesToSkip)
{
  const ezUInt32 uiBytes = ezMath::Min<ezUInt32>(static_cast<ezUInt32>(uiBytesToSkip), m_uiChunkSize - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void ezRawMemoryStreamReader::SetReadPosition(ezUInt32 uiReadPosition)
{
  EZ_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

ezUInt32 ezRawMemoryStreamReader::GetByteCount() const
{
  return m_uiChunkSize;
}

void ezRawMemoryStreamReader::SetDebugSourceInformation(const char* szDebugSourceInformation)
{
  m_DebugSourceInformation = szDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_MemoryStream);
