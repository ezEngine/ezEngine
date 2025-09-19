
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

using ezTypeVersion = ezUInt16;

template <ezUInt16 Size, typename AllocatorWrapper>
struct ezHybridString;

using ezString = ezHybridString<32, ezDefaultAllocatorWrapper>;

/// \brief Abstract base class for binary input streams providing unified reading interface.
///
/// StreamReader defines the fundamental interface for reading binary data from various sources
/// including files, memory buffers, network connections, and compressed streams. All read
/// operations are performed in a sequential manner with automatic endianness handling.
///
/// Core functionality:
/// - ReadBytes(): Raw binary data reading (pure virtual, must be implemented)
/// - Typed reading methods with endianness conversion (ReadWordValue, ReadDWordValue, etc.)
/// - High-level container reading (arrays, sets, maps, hash tables)
/// - String reading with automatic length handling
///
/// Implementation requirements:
/// - Derived classes must implement ReadBytes() as the primary read method
/// - All other methods are implemented in terms of ReadBytes()
/// - Should handle EOF conditions gracefully by returning actual bytes read
class EZ_FOUNDATION_DLL ezStreamReader
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStreamReader);

public:
  /// \brief Constructor
  ezStreamReader();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamReader();

  /// \brief Reads a raw number of bytes into the read buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pReadBuffer Destination buffer for the read data
  /// \param uiBytesToRead Maximum number of bytes to read
  /// \return Actual number of bytes read (may be less than requested on EOF or error)
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) = 0; // [tested]

  /// \brief Helper method to read a word value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult ReadWordValue(T* pWordValue); // [tested]

  /// \brief Helper method to read a dword value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult ReadDWordValue(T* pDWordValue); // [tested]

  /// \brief Helper method to read a qword value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult ReadQWordValue(T* pQWordValue); // [tested]

  /// \brief Reads an array of elements from the stream
  template <typename ArrayType, typename ValueType>
  ezResult ReadArray(ezArrayBase<ValueType, ArrayType>& inout_array); // [tested]

  /// \brief Reads a small array of elements from the stream
  template <typename ValueType, ezUInt16 uiSize, typename AllocatorWrapper>
  ezResult ReadArray(ezSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, ezUInt32 uiSize>
  ezResult ReadArray(ValueType (&array)[uiSize]);

  /// \brief Reads a set
  template <typename KeyType, typename Comparer>
  ezResult ReadSet(ezSetBase<KeyType, Comparer>& inout_set); // [tested]

  /// \brief Reads a map
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult ReadMap(ezMapBase<KeyType, ValueType, Comparer>& inout_map); // [tested]

  /// \brief Read a hash table (note that the entry order is not stable)
  template <typename KeyType, typename ValueType, typename Hasher>
  ezResult ReadHashTable(ezHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable); // [tested]

  /// \brief Reads a string into an ezStringBuilder
  ezResult ReadString(ezStringBuilder& ref_sBuilder); // [tested]

  /// \brief Reads a string into an ezString
  ezResult ReadString(ezString& ref_sString);


  /// \brief Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip)
  {
    ezUInt8 uiTempBuffer[1024];

    ezUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytesToSkip)
    {
      ezUInt64 uiBytesToRead = ezMath::Min<ezUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      ezUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }

  EZ_ALWAYS_INLINE ezTypeVersion ReadVersion(ezTypeVersion expectedMaxVersion);
};

/// \brief Abstract base class for binary output streams providing unified writing interface.
///
/// StreamWriter defines the fundamental interface for writing binary data to various destinations
/// including files, memory buffers, network connections, and compressed streams. All write
/// operations are performed sequentially with automatic endianness handling.
///
/// Core functionality:
/// - WriteBytes(): Raw binary data writing (pure virtual, must be implemented)
/// - Typed writing methods with endianness conversion (WriteWordValue, WriteDWordValue, etc.)
/// - High-level container writing (arrays, sets, maps, hash tables)
/// - String writing with automatic length prefixing
/// - Optional flush support for ensuring data persistence
///
/// Implementation requirements:
/// - Derived classes must implement WriteBytes() as the primary write method
/// - All other methods are implemented in terms of WriteBytes()
/// - Flush() can be overridden for buffered implementations
/// - Should handle write errors gracefully by returning appropriate ezResult
class EZ_FOUNDATION_DLL ezStreamWriter
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStreamWriter);

public:
  /// \brief Constructor
  ezStreamWriter();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamWriter();

  /// \brief Writes a raw number of bytes from the buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pWriteBuffer Source buffer containing data to write
  /// \param uiBytesToWrite Number of bytes to write from the buffer
  /// \return EZ_SUCCESS if all bytes were written successfully, EZ_FAILURE otherwise
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) = 0; // [tested]

  /// \brief Flushes buffered data to the underlying storage, ensuring data persistence.
  ///
  /// Default implementation is a no-op. Derived classes with internal buffering should
  /// override this method to force writing of buffered data to the actual destination.
  virtual ezResult Flush() // [tested]
  {
    return EZ_SUCCESS;
  }

  /// \brief Helper method to write a word value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult WriteWordValue(const T* pWordValue); // [tested]

  /// \brief Helper method to write a dword value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult WriteDWordValue(const T* pDWordValue); // [tested]

  /// \brief Helper method to write a qword value correctly (copes with potentially different endianess)
  template <typename T>
  ezResult WriteQWordValue(const T* pQWordValue); // [tested]

  /// \brief Writes a type version to the stream
  EZ_ALWAYS_INLINE void WriteVersion(ezTypeVersion version);

  /// \brief Writes an array of elements to the stream
  template <typename ArrayType, typename ValueType>
  ezResult WriteArray(const ezArrayBase<ValueType, ArrayType>& array); // [tested]

  /// \brief Writes a small array of elements to the stream
  template <typename ValueType, ezUInt16 uiSize>
  ezResult WriteArray(const ezSmallArrayBase<ValueType, uiSize>& array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, ezUInt32 uiSize>
  ezResult WriteArray(const ValueType (&array)[uiSize]);

  /// \brief Writes a set
  template <typename KeyType, typename Comparer>
  ezResult WriteSet(const ezSetBase<KeyType, Comparer>& set); // [tested]

  /// \brief Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult WriteMap(const ezMapBase<KeyType, ValueType, Comparer>& map); // [tested]

  /// \brief Writes a hash table (note that the entry order might change on read)
  template <typename KeyType, typename ValueType, typename Hasher>
  ezResult WriteHashTable(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable); // [tested]

  /// \brief Writes a string
  ezResult WriteString(const ezStringView sStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>
