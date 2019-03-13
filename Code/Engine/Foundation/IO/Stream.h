
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/Map.h>

typedef ezUInt16 ezTypeVersion;

/// \brief Interface for binary in (read) streams.
class EZ_FOUNDATION_DLL ezStreamReader
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStreamReader);
public:
  /// \brief Constructor
  ezStreamReader();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamReader();

  /// \brief Reads a raw number of bytes into the read buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
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
  ezResult ReadArray(ezArrayBase<ValueType, ArrayType>& Array); // [tested]

    /// \brief Writes a C style fixed array
  template <typename ValueType, ezUInt32 uiSize>
  ezResult ReadArray(ValueType (&Array)[uiSize]);

  /// \brief Reads a set
  template <typename KeyType, typename Comparer>
  ezResult ReadSet(ezSetBase<KeyType, Comparer>& Set); // [tested]

  /// \brief Reads a map
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult ReadMap(ezMapBase<KeyType, ValueType, Comparer>& Map); // [tested]

  /// \brief Reads a string into a ezStringBuilder
  ezResult ReadString(ezStringBuilder& builder); // [tested]

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

  EZ_ALWAYS_INLINE ezTypeVersion ReadVersion(ezTypeVersion uiExpectedMaxVersion);
};

/// \brief Interface for binary out (write) streams.
class EZ_FOUNDATION_DLL ezStreamWriter
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStreamWriter);
public:
  /// \brief Constructor
  ezStreamWriter();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamWriter();

  /// \brief Writes a raw number of bytes from the buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) = 0; // [tested]

  /// \brief Flushes the stream, may be implemented (not necessary to implement the interface correctly) so that user code can ensure that
  /// content is written
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
  EZ_ALWAYS_INLINE void WriteVersion(ezTypeVersion uiVersion);
  
  /// \brief Writes an array of elements to the stream
  template <typename ArrayType, typename ValueType>
  ezResult WriteArray(const ezArrayBase<ValueType, ArrayType>& Array); // [tested]

  /// \brief Writes a C style fixed array
  template <typename ValueType, ezUInt32 uiSize>
  ezResult WriteArray(const ValueType (&Array)[uiSize]);

  /// \brief Writes a set
  template <typename KeyType, typename Comparer>
  ezResult WriteSet(const ezSetBase<KeyType, Comparer>& Set); // [tested]

  /// \brief Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult WriteMap(const ezMapBase<KeyType, ValueType, Comparer>& Map); // [tested]

  /// \brief Writes a string
  ezResult WriteString(const ezStringView szStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>

