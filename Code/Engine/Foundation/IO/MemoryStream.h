#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

class ezMemoryStreamReader;
class ezMemoryStreamWriter;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Abstract interface for memory stream storage providing pluggable backend implementations.
///
/// MemoryStreamStorageInterface defines the contract for storage backends used by memory streams.
/// Different implementations can optimize for specific use cases such as small temporary buffers,
/// large datasets, or zero-copy operations with existing containers.
class EZ_FOUNDATION_DLL ezMemoryStreamStorageInterface
{
public:
  ezMemoryStreamStorageInterface();
  virtual ~ezMemoryStreamStorageInterface();

  /// \brief Returns the number of bytes that are currently stored. Asserts that the stored amount is less than 4GB.
  ezUInt32 GetStorageSize32() const
  {
    EZ_ASSERT_ALWAYS(GetStorageSize64() <= ezMath::MaxValue<ezUInt32>(), "The memory stream storage object has grown beyond 4GB. The code using it has to be adapted to support this.");
    return (ezUInt32)GetStorageSize64();
  }

  /// \brief Returns the number of bytes that are currently stored.
  virtual ezUInt64 GetStorageSize64() const = 0; // [tested]

  /// \brief Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  virtual void Clear() = 0;

  /// \brief Deallocates any allocated memory that's not needed to hold the currently stored data.
  virtual void Compact() = 0;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  virtual ezUInt64 GetHeapMemoryUsage() const = 0;

  /// \brief Copies all data from the given stream into the storage.
  void ReadAll(ezStreamReader& inout_stream, ezUInt64 uiMaxBytes = ezMath::MaxValue<ezUInt64>());

  /// \brief Reserves N bytes of storage.
  virtual void Reserve(ezUInt64 uiBytes) = 0;

  /// \brief Writes the entire content of the storage to the provided stream.
  virtual ezResult CopyToStream(ezStreamWriter& inout_stream) const = 0;

  /// \brief Returns a read-only ezArrayPtr that represents a contiguous area in memory which starts at the given first byte.
  ///
  /// This piece of memory can be read/copied/modified in one operation (memcpy etc).
  /// The next byte after this slice may be located somewhere entirely different in memory.
  /// Call GetContiguousMemoryRange() again with the next byte after this range, to get access to the next memory area.
  ///
  /// Chunks may differ in size.
  virtual ezArrayPtr<const ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) const = 0;

  /// Non-const overload of GetContiguousMemoryRange().
  virtual ezArrayPtr<ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) = 0;

private:
  virtual void SetInternalSize(ezUInt64 uiSize) = 0;

  friend class ezMemoryStreamReader;
  friend class ezMemoryStreamWriter;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Templated implementation of ezMemoryStreamStorageInterface that adapts most standard ez containers to the interface.
///
/// Note that ezMemoryStreamContainerStorage assumes contiguous storage, so using an ezDeque for storage will not work.
template <typename CONTAINER>
class ezMemoryStreamContainerStorage : public ezMemoryStreamStorageInterface
{
public:
  /// \brief Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve some memory up front.
  ezMemoryStreamContainerStorage(ezUInt32 uiInitialCapacity = 0, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator())
    : m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  virtual ezUInt64 GetStorageSize64() const override { return m_Storage.GetCount(); }
  virtual void Clear() override { m_Storage.Clear(); }
  virtual void Compact() override { m_Storage.Compact(); }
  virtual ezUInt64 GetHeapMemoryUsage() const override { return m_Storage.GetHeapMemoryUsage(); }

  virtual void Reserve(ezUInt64 uiBytes) override
  {
    EZ_ASSERT_DEV(uiBytes <= ezMath::MaxValue<ezUInt32>(), "ezMemoryStreamContainerStorage only supports 32 bit addressable sizes.");
    m_Storage.Reserve(static_cast<ezUInt32>(uiBytes));
  }

  virtual ezResult CopyToStream(ezStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_Storage.GetData(), m_Storage.GetCount());
  }

  virtual ezArrayPtr<const ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return ezArrayPtr<const ezUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<ezUInt32>(uiStartByte));
  }

  virtual ezArrayPtr<ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return ezArrayPtr<ezUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<ezUInt32>(uiStartByte));
  }

  /// \brief The data is guaranteed to be contiguous.
  const ezUInt8* GetData() const { return m_Storage.GetData(); }

private:
  virtual void SetInternalSize(ezUInt64 uiSize) override
  {
    EZ_ASSERT_DEV(uiSize <= ezMath::MaxValue<ezUInt32>(), "Storage that large is not supported.");
    m_Storage.SetCountUninitialized(static_cast<ezUInt32>(uiSize));
  }

  CONTAINER m_Storage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// ezContiguousMemoryStreamStorage holds internally an ezHybridArray<ezUInt8, 256>, to prevent allocations when only small temporary memory streams
/// are needed. That means it will have a memory overhead of that size.
/// Also it reallocates memory on demand, and the data is guaranteed to be contiguous. This may be desirable,
/// but can have a high performance overhead when data grows very large.
class EZ_FOUNDATION_DLL ezContiguousMemoryStreamStorage : public ezMemoryStreamContainerStorage<ezHybridArray<ezUInt8, 256>>
{
public:
  ezContiguousMemoryStreamStorage(ezUInt32 uiInitialCapacity = 0, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator())
    : ezMemoryStreamContainerStorage<ezHybridArray<ezUInt8, 256>>(uiInitialCapacity, pAllocator)
  {
  }
};

/// \brief The default implementation for memory stream storage.
///
/// This implementation of ezMemoryStreamStorageInterface handles use cases both from very small to extremely large storage needs.
/// It starts out with some inplace memory that can accommodate small amounts of data.
/// To grow, additional chunks of data are allocated. No memory ever needs to be copied to grow the container.
/// However, that also means that the memory isn't stored in one contiguous array, therefore data has to be accessed piece-wise
/// through GetContiguousMemoryRange().
class EZ_FOUNDATION_DLL ezDefaultMemoryStreamStorage final : public ezMemoryStreamStorageInterface
{
public:
  ezDefaultMemoryStreamStorage(ezUInt32 uiInitialCapacity = 0, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ~ezDefaultMemoryStreamStorage();

  virtual void Reserve(ezUInt64 uiBytes) override;    // [tested]

  virtual ezUInt64 GetStorageSize64() const override; // [tested]
  virtual void Clear() override;
  virtual void Compact() override;
  virtual ezUInt64 GetHeapMemoryUsage() const override;
  virtual ezResult CopyToStream(ezStreamWriter& inout_stream) const override;
  virtual ezArrayPtr<const ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) const override; // [tested]
  virtual ezArrayPtr<ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) override;             // [tested]

private:
  virtual void SetInternalSize(ezUInt64 uiSize) override;

  void AddChunk(ezUInt32 uiMinimumSize);

  struct Chunk
  {
    ezUInt64 m_uiStartOffset = 0;
    ezArrayPtr<ezUInt8> m_Bytes;
  };

  ezHybridArray<Chunk, 16> m_Chunks;

  ezUInt64 m_uiCapacity = 0;
  ezUInt64 m_uiInternalSize = 0;
  ezUInt8 m_InplaceMemory[512]; // used for the very first bytes, might cover small memory streams without an allocation
  mutable ezUInt32 m_uiLastChunkAccessed = 0;
  mutable ezUInt64 m_uiLastByteAccessed = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Wrapper around an existing container to implement ezMemoryStreamStorageInterface
template <typename CONTAINER>
class ezMemoryStreamContainerWrapperStorage : public ezMemoryStreamStorageInterface
{
public:
  ezMemoryStreamContainerWrapperStorage(CONTAINER* pContainer) { m_pStorage = pContainer; }

  virtual ezUInt64 GetStorageSize64() const override { return m_pStorage->GetCount(); }
  virtual void Clear() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Clear();
    }
  }
  virtual void Compact() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Compact();
    }
  }

  virtual ezUInt64 GetHeapMemoryUsage() const override { return m_pStorage->GetHeapMemoryUsage(); }

  virtual void Reserve(ezUInt64 uiBytes) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      EZ_ASSERT_DEV(uiBytes <= ezMath::MaxValue<ezUInt32>(), "ezMemoryStreamContainerWrapperStorage only supports 32 bit addressable sizes.");
      m_pStorage->Reserve(static_cast<ezUInt32>(uiBytes));
    }
  }

  virtual ezResult CopyToStream(ezStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_pStorage->GetData(), m_pStorage->GetCount());
  }

  virtual ezArrayPtr<const ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_pStorage->GetCount())
      return {};

    return ezArrayPtr<const ezUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<ezUInt32>(uiStartByte));
  }

  virtual ezArrayPtr<ezUInt8> GetContiguousMemoryRange(ezUInt64 uiStartByte) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      if (uiStartByte >= m_pStorage->GetCount())
        return {};

      return ezArrayPtr<ezUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<ezUInt32>(uiStartByte));
    }
    else
    {
      return {};
    }
  }

private:
  virtual void SetInternalSize(ezUInt64 uiSize) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      EZ_ASSERT_DEV(uiSize <= ezMath::MaxValue<ezUInt32>(), "ezMemoryStreamContainerWrapperStorage only supports up to 4GB sizes.");
      m_pStorage->SetCountUninitialized(static_cast<ezUInt32>(uiSize));
    }
  }

  CONTAINER* m_pStorage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A reader which can access a memory stream.
///
/// MemoryStreamReader provides sequential reading access to data stored in any MemoryStreamStorageInterface
/// implementation. It maintains an internal read position and supports both forward reading and seeking.
///
/// Thread safety:
/// - NOT thread-safe - each thread requires its own reader instance
/// - Multiple readers can safely share the same storage concurrently
/// - Concurrent reading and writing to the same storage is unsafe
class EZ_FOUNDATION_DLL ezMemoryStreamReader : public ezStreamReader
{
public:
  /// \brief Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  ezMemoryStreamReader(const ezMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~ezMemoryStreamReader();

  /// \brief Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(const ezMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiReadPosition = 0;
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(ezUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position
  ezUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  ezUInt32 GetByteCount32() const; // [tested]
  ezUInt64 GetByteCount64() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(ezStringView sDebugSourceInformation);

private:
  const ezMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  ezString m_sDebugSourceInformation;

  ezUInt64 m_uiReadPosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A writer which can access a memory stream
///
/// MemoryStreamWriter provides sequential writing access to any MemoryStreamStorageInterface
/// implementation. It automatically grows the underlying storage as needed and maintains
/// an internal write position that can be positioned anywhere within the stream.
///
/// Thread safety:
/// - NOT thread-safe - requires exclusive access
/// - Cannot safely share writer instances between threads
/// - Concurrent writing and reading to the same storage is unsafe
class EZ_FOUNDATION_DLL ezMemoryStreamWriter : public ezStreamWriter
{
public:
  /// \brief Pass the memory storage object to which to write to.
  ezMemoryStreamWriter(ezMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~ezMemoryStreamWriter();

  /// \brief Sets the storage object upon which to operate. Resets the write position to the end of the storage stream.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(ezMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiWritePosition = 0;
    if (m_pStreamStorage)
      m_uiWritePosition = m_pStreamStorage->GetStorageSize64();
  }

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Sets the write position to be used
  void SetWritePosition(ezUInt64 uiWritePosition); // [tested]

  /// \brief Returns the current write position
  ezUInt64 GetWritePosition() const { return m_uiWritePosition; }

  /// \brief Returns the total stored bytes in the memory stream
  ezUInt32 GetByteCount32() const; // [tested]
  ezUInt64 GetByteCount64() const; // [tested]

private:
  ezMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  ezUInt64 m_uiWritePosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Maps a raw chunk of memory to the ezStreamReader interface.
///
/// RawMemoryStreamReader provides direct read access to a pre-existing memory buffer without
/// requiring any storage interface or memory management. It's optimized for scenarios where
/// you have a fixed chunk of memory (array, buffer, etc.) and need stream interface access.
class EZ_FOUNDATION_DLL ezRawMemoryStreamReader : public ezStreamReader
{
public:
  ezRawMemoryStreamReader();

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  ezRawMemoryStreamReader(const void* pData, ezUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard ez container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  ezRawMemoryStreamReader(const CONTAINER& container) // [tested]
  {
    Reset(container);
  }

  ~ezRawMemoryStreamReader();

  void Reset(const void* pData, ezUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(const CONTAINER& container)              // [tested]
  {
    Reset(static_cast<const ezUInt8*>(container.GetData()), container.GetCount());
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(ezUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position in the raw memory block
  ezUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  ezUInt64 GetByteCount() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(ezStringView sDebugSourceInformation);

private:
  const ezUInt8* m_pRawMemory = nullptr;

  ezUInt64 m_uiChunkSize = 0;
  ezUInt64 m_uiReadPosition = 0;

  ezString m_sDebugSourceInformation;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// \brief Maps a raw chunk of memory to the ezStreamReader interface.
class EZ_FOUNDATION_DLL ezRawMemoryStreamWriter : public ezStreamWriter
{
public:
  ezRawMemoryStreamWriter(); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  ezRawMemoryStreamWriter(void* pData, ezUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard ez container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  ezRawMemoryStreamWriter(CONTAINER& ref_container) // [tested]
  {
    Reset(ref_container);
  }

  ~ezRawMemoryStreamWriter();                   // [tested]

  void Reset(void* pData, ezUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(CONTAINER& ref_container)          // [tested]
  {
    Reset(static_cast<ezUInt8*>(ref_container.GetData()), ref_container.GetCount());
  }

  /// \brief Returns the total available bytes in the memory stream
  ezUInt64 GetStorageSize() const; // [tested]

  /// \brief Returns the number of bytes written to the storage
  ezUInt64 GetNumWrittenBytes() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(ezStringView sDebugSourceInformation);

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

private:
  ezUInt8* m_pRawMemory = nullptr;

  ezUInt64 m_uiChunkSize = 0;
  ezUInt64 m_uiWritePosition = 0;

  ezString m_sDebugSourceInformation;
};
