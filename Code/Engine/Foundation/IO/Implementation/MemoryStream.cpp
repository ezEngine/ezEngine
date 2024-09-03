#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>

// Reader implementation

ezMemoryStreamReader::ezMemoryStreamReader(const ezMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)
{
}

ezMemoryStreamReader::~ezMemoryStreamReader() = default;

ezUInt64 ezMemoryStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const ezUInt64 uiBytes = ezMath::Min<ezUInt64>(uiBytesToRead, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    ezUInt64 uiBytesLeft = uiBytes;

    while (uiBytesLeft > 0)
    {
      ezArrayPtr<const ezUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiReadPosition);

      EZ_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const ezUInt64 toRead = ezMath::Min<ezUInt64>(data.GetCount(), uiBytesLeft);

      ezMemoryUtils::Copy(static_cast<ezUInt8*>(pReadBuffer), data.GetPtr(), static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      pReadBuffer = ezMemoryUtils::AddByteOffset(pReadBuffer, static_cast<size_t>(toRead));                // Down-cast to size_t for 32-bit.

      m_uiReadPosition += toRead;
      uiBytesLeft -= toRead;
    }
  }
  else
  {
    m_uiReadPosition += uiBytes;
  }

  return uiBytes;
}

ezUInt64 ezMemoryStreamReader::SkipBytes(ezUInt64 uiBytesToSkip)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const ezUInt64 uiBytes = ezMath::Min<ezUInt64>(uiBytesToSkip, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void ezMemoryStreamReader::SetReadPosition(ezUInt64 uiReadPosition)
{
  EZ_ASSERT_RELEASE(uiReadPosition <= GetByteCount64(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

ezUInt32 ezMemoryStreamReader::GetByteCount32() const
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize32();
}

ezUInt64 ezMemoryStreamReader::GetByteCount64() const
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize64();
}

void ezMemoryStreamReader::SetDebugSourceInformation(ezStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////

// Writer implementation
ezMemoryStreamWriter::ezMemoryStreamWriter(ezMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)

{
}

ezMemoryStreamWriter::~ezMemoryStreamWriter() = default;

ezResult ezMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  EZ_ASSERT_DEV(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  if (uiBytesToWrite == 0)
    return EZ_SUCCESS;

  EZ_ASSERT_DEBUG(pWriteBuffer != nullptr, "No valid buffer containing data given!");

  // Reserve the memory in the storage object, grow size if appending data (don't shrink)
  m_pStreamStorage->SetInternalSize(ezMath::Max(m_pStreamStorage->GetStorageSize64(), m_uiWritePosition + uiBytesToWrite));

  {
    ezUInt64 uiBytesLeft = uiBytesToWrite;

    while (uiBytesLeft > 0)
    {
      ezArrayPtr<ezUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiWritePosition);

      EZ_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const ezUInt64 toWrite = ezMath::Min<ezUInt64>(data.GetCount(), uiBytesLeft);

      ezMemoryUtils::Copy(data.GetPtr(), static_cast<const ezUInt8*>(pWriteBuffer), static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      pWriteBuffer = ezMemoryUtils::AddByteOffset(pWriteBuffer, static_cast<size_t>(toWrite));                     // Down-cast to size_t for 32-bit.

      m_uiWritePosition += toWrite;
      uiBytesLeft -= toWrite;
    }
  }

  return EZ_SUCCESS;
}

void ezMemoryStreamWriter::SetWritePosition(ezUInt64 uiWritePosition)
{
  EZ_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  EZ_ASSERT_RELEASE(uiWritePosition <= GetByteCount64(), "Write position must be between 0 and GetByteCount()!");
  m_uiWritePosition = uiWritePosition;
}

ezUInt32 ezMemoryStreamWriter::GetByteCount32() const
{
  EZ_ASSERT_DEV(m_uiWritePosition <= 0xFFFFFFFFllu, "Use GetByteCount64 instead of GetByteCount32");
  return (ezUInt32)m_uiWritePosition;
}

ezUInt64 ezMemoryStreamWriter::GetByteCount64() const
{
  return m_uiWritePosition;
}

//////////////////////////////////////////////////////////////////////////

ezMemoryStreamStorageInterface::ezMemoryStreamStorageInterface() = default;
ezMemoryStreamStorageInterface::~ezMemoryStreamStorageInterface() = default;

void ezMemoryStreamStorageInterface::ReadAll(ezStreamReader& inout_stream, ezUInt64 uiMaxBytes /*= 0xFFFFFFFFFFFFFFFFllu*/)
{
  Clear();
  ezMemoryStreamWriter w(this);

  ezUInt8 uiTemp[1024 * 8];

  while (uiMaxBytes > 0)
  {
    const ezUInt64 uiToRead = ezMath::Min<ezUInt64>(uiMaxBytes, EZ_ARRAY_SIZE(uiTemp));

    const ezUInt64 uiRead = inout_stream.ReadBytes(uiTemp, uiToRead);
    uiMaxBytes -= uiRead;

    w.WriteBytes(uiTemp, uiRead).IgnoreResult();

    if (uiRead < uiToRead)
      break;
  }
}

//////////////////////////////////////////////////////////////////////////


ezRawMemoryStreamReader::ezRawMemoryStreamReader() = default;

ezRawMemoryStreamReader::ezRawMemoryStreamReader(const void* pData, ezUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

ezRawMemoryStreamReader::~ezRawMemoryStreamReader() = default;

void ezRawMemoryStreamReader::Reset(const void* pData, ezUInt64 uiDataSize)
{
  m_pRawMemory = static_cast<const ezUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiReadPosition = 0;
}

ezUInt64 ezRawMemoryStreamReader::ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
{
  const ezUInt64 uiBytes = ezMath::Min<ezUInt64>(uiBytesToRead, m_uiChunkSize - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    ezMemoryUtils::Copy(static_cast<ezUInt8*>(pReadBuffer), &m_pRawMemory[m_uiReadPosition], static_cast<size_t>(uiBytes));
  }

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

ezUInt64 ezRawMemoryStreamReader::SkipBytes(ezUInt64 uiBytesToSkip)
{
  const ezUInt64 uiBytes = ezMath::Min<ezUInt64>(uiBytesToSkip, m_uiChunkSize - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void ezRawMemoryStreamReader::SetReadPosition(ezUInt64 uiReadPosition)
{
  EZ_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

ezUInt64 ezRawMemoryStreamReader::GetByteCount() const
{
  return m_uiChunkSize;
}

void ezRawMemoryStreamReader::SetDebugSourceInformation(ezStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////


ezRawMemoryStreamWriter::ezRawMemoryStreamWriter() = default;

ezRawMemoryStreamWriter::ezRawMemoryStreamWriter(void* pData, ezUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

ezRawMemoryStreamWriter::~ezRawMemoryStreamWriter() = default;

void ezRawMemoryStreamWriter::Reset(void* pData, ezUInt64 uiDataSize)
{
  EZ_ASSERT_DEV(pData != nullptr, "Invalid memory stream storage");

  m_pRawMemory = static_cast<ezUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiWritePosition = 0;
}

ezResult ezRawMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  const ezUInt64 uiBytes = ezMath::Min<ezUInt64>(uiBytesToWrite, m_uiChunkSize - m_uiWritePosition);

  ezMemoryUtils::Copy(&m_pRawMemory[m_uiWritePosition], static_cast<const ezUInt8*>(pWriteBuffer), static_cast<size_t>(uiBytes));

  m_uiWritePosition += uiBytes;

  if (uiBytes < uiBytesToWrite)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezUInt64 ezRawMemoryStreamWriter::GetStorageSize() const
{
  return m_uiChunkSize;
}

ezUInt64 ezRawMemoryStreamWriter::GetNumWrittenBytes() const
{
  return m_uiWritePosition;
}

void ezRawMemoryStreamWriter::SetDebugSourceInformation(ezStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezDefaultMemoryStreamStorage::ezDefaultMemoryStreamStorage(ezUInt32 uiInitialCapacity, ezAllocator* pAllocator)
  : m_Chunks(pAllocator)
{
  Reserve(uiInitialCapacity);
}

ezDefaultMemoryStreamStorage::~ezDefaultMemoryStreamStorage()
{
  Clear();
}

void ezDefaultMemoryStreamStorage::Reserve(ezUInt64 uiBytes)
{
  if (m_Chunks.IsEmpty())
  {
    auto& chunk = m_Chunks.ExpandAndGetRef();
    chunk.m_Bytes = ezByteArrayPtr(m_InplaceMemory);
    chunk.m_uiStartOffset = 0;
    m_uiCapacity = m_Chunks[0].m_Bytes.GetCount();
  }

  while (m_uiCapacity < uiBytes)
  {
    AddChunk(static_cast<ezUInt32>(ezMath::Min<ezUInt64>(uiBytes - m_uiCapacity, ezMath::MaxValue<ezUInt32>())));
  }
}

ezUInt64 ezDefaultMemoryStreamStorage::GetStorageSize64() const
{
  return m_uiInternalSize;
}

void ezDefaultMemoryStreamStorage::Clear()
{
  m_uiInternalSize = 0;
  m_uiLastByteAccessed = 0;
  m_uiLastChunkAccessed = 0;
  Compact();
}

void ezDefaultMemoryStreamStorage::Compact()
{
  // skip chunk 0, because that's where our inplace storage is used
  while (m_Chunks.GetCount() > 1)
  {
    auto& chunk = m_Chunks.PeekBack();

    if (m_uiInternalSize > m_uiCapacity - chunk.m_Bytes.GetCount())
      break;

    m_uiCapacity -= chunk.m_Bytes.GetCount();

    ezUInt8* pData = chunk.m_Bytes.GetPtr();
    EZ_DELETE_RAW_BUFFER(m_Chunks.GetAllocator(), pData);

    m_Chunks.PopBack();
  }
}

ezUInt64 ezDefaultMemoryStreamStorage::GetHeapMemoryUsage() const
{
  return m_Chunks.GetHeapMemoryUsage() + m_uiCapacity - m_Chunks[0].m_Bytes.GetCount();
}

ezResult ezDefaultMemoryStreamStorage::CopyToStream(ezStreamWriter& inout_stream) const
{
  ezUInt64 uiBytesLeft = m_uiInternalSize;
  ezUInt64 uiReadPosition = 0;

  while (uiBytesLeft > 0)
  {
    ezArrayPtr<const ezUInt8> data = GetContiguousMemoryRange(uiReadPosition);

    EZ_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(data.GetPtr(), data.GetCount()));

    uiReadPosition += data.GetCount();
    uiBytesLeft -= data.GetCount();
  }

  return EZ_SUCCESS;
}

ezArrayPtr<const ezUInt8> ezDefaultMemoryStreamStorage::GetContiguousMemoryRange(ezUInt64 uiStartByte) const
{
  if (uiStartByte >= m_uiInternalSize)
    return {};

  // remember the last access (byte offset) and in which chunk that ended up, to speed up this lookup
  // if a read comes in that's not AFTER the previous one, just reset to the start

  if (uiStartByte < m_uiLastByteAccessed)
  {
    m_uiLastChunkAccessed = 0;
  }

  m_uiLastByteAccessed = uiStartByte;

  for (; m_uiLastChunkAccessed < m_Chunks.GetCount(); ++m_uiLastChunkAccessed)
  {
    const auto& chunk = m_Chunks[m_uiLastChunkAccessed];

    if (uiStartByte < chunk.m_uiStartOffset + chunk.m_Bytes.GetCount())
    {
      const ezUInt64 uiStartByteRel = uiStartByte - chunk.m_uiStartOffset;    // start offset into the chunk
      const ezUInt64 uiMaxLenRel = chunk.m_Bytes.GetCount() - uiStartByteRel; // max number of bytes to use from this chunk
      const ezUInt64 uiMaxRangeRel = m_uiInternalSize - uiStartByte;          // the 'stored data' might be less than the capacity of the chunk

      return {chunk.m_Bytes.GetPtr() + uiStartByteRel, static_cast<ezUInt32>(ezMath::Min<ezUInt64>(uiMaxRangeRel, uiMaxLenRel))};
    }
  }

  return {};
}

ezArrayPtr<ezUInt8> ezDefaultMemoryStreamStorage::GetContiguousMemoryRange(ezUInt64 uiStartByte)
{
  ezArrayPtr<const ezUInt8> constData = const_cast<const ezDefaultMemoryStreamStorage*>(this)->GetContiguousMemoryRange(uiStartByte);
  return {const_cast<ezUInt8*>(constData.GetPtr()), constData.GetCount()};
}

void ezDefaultMemoryStreamStorage::SetInternalSize(ezUInt64 uiSize)
{
  Reserve(uiSize);

  m_uiInternalSize = uiSize;
}

void ezDefaultMemoryStreamStorage::AddChunk(ezUInt32 uiMinimumSize)
{
  auto& chunk = m_Chunks.ExpandAndGetRef();

  ezUInt32 uiSize = 0;

  if (m_Chunks.GetCount() < 4)
  {
    uiSize = 1024 * 4; // 4 KB
  }
  else if (m_Chunks.GetCount() < 8)
  {
    uiSize = 1024 * 64; // 64 KB
  }
  else if (m_Chunks.GetCount() < 16)
  {
    uiSize = 1024 * 1024 * 4; // 4 MB
  }
  else
  {
    uiSize = 1024 * 1024 * 64; // 64 MB
  }

  uiSize = ezMath::Max(uiSize, uiMinimumSize);

  const auto& prevChunk = m_Chunks[m_Chunks.GetCount() - 2];

  chunk.m_Bytes = ezArrayPtr<ezUInt8>(EZ_NEW_RAW_BUFFER(m_Chunks.GetAllocator(), ezUInt8, uiSize), uiSize);
  chunk.m_uiStartOffset = prevChunk.m_uiStartOffset + prevChunk.m_Bytes.GetCount();
  m_uiCapacity += chunk.m_Bytes.GetCount();
}
