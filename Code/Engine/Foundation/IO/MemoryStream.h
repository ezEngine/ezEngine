
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/RefCounted.h>

class ezMemoryStreamReader;
class ezMemoryStreamWriter;

/// \brief Instances of this class act as storage for memory streams
///
/// ezMemoryStreamStorage holds internally an ezHybridArray<ezUInt8, 256>, to prevent allocations when only small temporary memory streams are needed.
/// That means it will have a memory overhead of that size.
class ezMemoryStreamStorage : public ezRefCounted
{
public:
  /// \brief Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve a some memory up front, to reduce reallocations.
  ezMemoryStreamStorage(ezUInt32 uiInitialCapacity = 0, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator())
    : m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  ~ezMemoryStreamStorage()
  {
    EZ_ASSERT_RELEASE(!IsReferenced(), "Memory stream storage destroyed while there are still references by reader / writer object(s)!");
  }

  /// \brief Returns the number of bytes that is currently stored.
  ezUInt32 GetStorageSize() const { return m_Storage.GetCount(); }

  /// \brief Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  void Clear() { m_Storage.Clear(); }

  const ezUInt8* GetData() const { if (m_Storage.IsEmpty()) return nullptr; return &m_Storage[0]; }

private:
  friend class ezMemoryStreamReader;
  friend class ezMemoryStreamWriter;

  ezHybridArray<ezUInt8, 256> m_Storage;
};

/// \brief A reader which can access a memory stream.
///
/// Please note that the functions exposed by this object are not thread safe! If access to the same ezMemoryStreamStorage object from
/// multiple threads is desired please create one instance of ezMemoryStreamReader per thread.
class EZ_FOUNDATION_DLL ezMemoryStreamReader : public ezStreamReaderBase
{
public:
  /// \brief Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  ezMemoryStreamReader(ezMemoryStreamStorage* pStreamStorage = nullptr);

  ~ezMemoryStreamReader();

  /// \brief Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(ezMemoryStreamStorage* pStreamStorage) { m_pStreamStorage = pStreamStorage; m_uiReadPosition = 0; }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip); // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(ezUInt32 uiReadPosition); // [tested]

  /// \brief Returns the total available bytes in the memory stream
  ezUInt32 GetByteCount() const; // [tested]

private:
  
  ezScopedRefPointer<ezMemoryStreamStorage> m_pStreamStorage;

  ezUInt32 m_uiReadPosition;
};

/// \brief A writer which can access a memory stream
///
/// Please note that the functions exposed by this object are not thread safe!
class EZ_FOUNDATION_DLL ezMemoryStreamWriter : public ezStreamWriterBase
{
public:
  /// \brief Pass the memory storage object to which to write to.
  ezMemoryStreamWriter(ezMemoryStreamStorage* pStreamStorage = nullptr);

  ~ezMemoryStreamWriter();

  /// \brief Sets the storage object upon which to operate. Resets the write position to the end of the storage stream.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(ezMemoryStreamStorage* pStreamStorage)
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

  ezScopedRefPointer<ezMemoryStreamStorage> m_pStreamStorage;

  ezUInt32 m_uiWritePosition;
};

