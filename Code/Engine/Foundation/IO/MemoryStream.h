
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

class ezMemoryStreamReader;
class ezMemoryStreamWriter;

/// \brief Instances of this class act as storage for memory streams
class EZ_FOUNDATION_DLL ezMemoryStreamStorageInterface : public ezRefCounted
{
public:
  ezMemoryStreamStorageInterface();
  virtual ~ezMemoryStreamStorageInterface();

  /// \brief Returns the number of bytes that is currently stored.
  virtual ezUInt32 GetStorageSize() const = 0;

  /// \brief Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  virtual void Clear() = 0;

  /// \brief Calls Compact() on the internal array.
  virtual void Compact() = 0;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  virtual ezUInt64 GetHeapMemoryUsage() const = 0;

  /// \brief Returns a pointer to the internal data.
  virtual const ezUInt8* GetData() const = 0;

  /// \brief Copies all data from the given stream into the storage.
  void ReadAll(ezStreamReader& Stream);

private:
  virtual const ezUInt8* GetInternalData() const = 0;
  virtual ezUInt8* GetInternalData() = 0;
  virtual void SetInternalSize(ezUInt32 uiSize) = 0;

  friend class ezMemoryStreamReader;
  friend class ezMemoryStreamWriter;
};

/// \brief Templated implementation of ezMemoryStreamStorageInterface that adapts all standard ez containers to the interface.
///
/// Note that ezMemoryStreamReader and ezMemoryStreamWriter assume contiguous storage, so using an ezDeque for storage will not work.
template <typename CONTAINER>
class ezMemoryStreamContainerStorage : public ezMemoryStreamStorageInterface
{
public:
  /// \brief Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve a some memory up front, to reduce
  /// reallocations.
  ezMemoryStreamContainerStorage(ezUInt32 uiInitialCapacity = 0, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator())
      : m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  virtual ezUInt32 GetStorageSize() const override { return m_Storage.GetCount(); }
  virtual void Clear() override { m_Storage.Clear(); }
  virtual void Compact() override { m_Storage.Compact(); }
  virtual ezUInt64 GetHeapMemoryUsage() const override { return m_Storage.GetHeapMemoryUsage(); }
  virtual const ezUInt8* GetData() const override
  {
    if (m_Storage.IsEmpty())
      return nullptr;
    return &m_Storage[0];
  }

private:
  virtual const ezUInt8* GetInternalData() const override { return &m_Storage[0]; }
  virtual ezUInt8* GetInternalData() override { return &m_Storage[0]; }
  virtual void SetInternalSize(ezUInt32 uiSize) override { m_Storage.SetCountUninitialized(uiSize); }

  CONTAINER m_Storage;
};

/// ezMemoryStreamStorage holds internally an ezHybridArray<ezUInt8, 256>, to prevent allocations when only small temporary memory streams
/// are needed. That means it will have a memory overhead of that size.
class EZ_FOUNDATION_DLL ezMemoryStreamStorage : public ezMemoryStreamContainerStorage<ezHybridArray<ezUInt8, 256>>
{
public:
  ezMemoryStreamStorage(ezUInt32 uiInitialCapacity = 0, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator())
      : ezMemoryStreamContainerStorage<ezHybridArray<ezUInt8, 256>>(uiInitialCapacity, pAllocator)
  {
  }
};

/// \brief Wrapper around an existing container to implement ezMemoryStreamStorageInterface
template <typename CONTAINER>
class ezMemoryStreamContainerWrapperStorage : public ezMemoryStreamStorageInterface
{
public:
  ezMemoryStreamContainerWrapperStorage(CONTAINER* pContainer) { m_pStorage = pContainer; }

  virtual ezUInt32 GetStorageSize() const override { return m_pStorage->GetCount(); }
  virtual void Clear() override { m_pStorage->Clear(); }
  virtual void Compact() override { m_pStorage->Compact(); }
  virtual ezUInt64 GetHeapMemoryUsage() const override { return m_pStorage->GetHeapMemoryUsage(); }
  virtual const ezUInt8* GetData() const override
  {
    if (m_pStorage->IsEmpty())
      return nullptr;
    return &(*m_pStorage)[0];
  }

private:
  virtual const ezUInt8* GetInternalData() const override { return &(*m_pStorage)[0]; }
  virtual ezUInt8* GetInternalData() override { return &(*m_pStorage)[0]; }
  virtual void SetInternalSize(ezUInt32 uiSize) override { m_pStorage->SetCountUninitialized(uiSize); }

  CONTAINER* m_pStorage;
};

/// \brief A reader which can access a memory stream.
///
/// Please note that the functions exposed by this object are not thread safe! If access to the same ezMemoryStreamStorage object from
/// multiple threads is desired please create one instance of ezMemoryStreamReader per thread.
class EZ_FOUNDATION_DLL ezMemoryStreamReader : public ezStreamReader
{
public:
  /// \brief Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  ezMemoryStreamReader(ezMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~ezMemoryStreamReader();

  /// \brief Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(ezMemoryStreamStorageInterface* pStreamStorage)
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
  void SetReadPosition(ezUInt32 uiReadPosition); // [tested]

  /// \brief Returns the total available bytes in the memory stream
  ezUInt32 GetByteCount() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(const char* szDebugSourceInformation);

private:
  ezScopedRefPointer<ezMemoryStreamStorageInterface> m_pStreamStorage;

  ezString m_DebugSourceInformation;

  ezUInt32 m_uiReadPosition;
};

/// \brief A writer which can access a memory stream
///
/// Please note that the functions exposed by this object are not thread safe!
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
      m_uiWritePosition = m_pStreamStorage->GetStorageSize();
  }

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Sets the write position to be used
  void SetWritePosition(ezUInt32 uiReadPosition); // [tested]

  /// \brief Returns the total stored bytes in the memory stream
  ezUInt32 GetByteCount() const; // [tested]

private:
  ezScopedRefPointer<ezMemoryStreamStorageInterface> m_pStreamStorage;

  ezUInt32 m_uiWritePosition;
};


/// \brief Maps a raw chunk of memory to the ezStreamReader interface.
class EZ_FOUNDATION_DLL ezRawMemoryStreamReader : public ezStreamReader
{
public:
  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  ezRawMemoryStreamReader(const void* pData, ezUInt32 uiDataSize);

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard ez container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  ezRawMemoryStreamReader(const CONTAINER& container) // [tested]
  {
    m_pRawMemory = static_cast<const ezUInt8*>(container.GetData());
    m_uiChunkSize = container.GetCount();
    m_uiReadPosition = 0;
  }

  ~ezRawMemoryStreamReader();

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(ezUInt32 uiReadPosition); // [tested]

  /// \brief Returns the total available bytes in the memory stream
  ezUInt32 GetByteCount() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(const char* szDebugSourceInformation);

private:
  const ezUInt8* m_pRawMemory;

  ezUInt32 m_uiChunkSize;
  ezUInt32 m_uiReadPosition;

  ezString m_DebugSourceInformation;
};
