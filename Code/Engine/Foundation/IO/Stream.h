
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

/// \brief Interface for binary in (read) streams.
class EZ_FOUNDATION_DLL ezStreamReaderBase 
{

public:

  /// \brief Constructor
  ezStreamReaderBase()
  {
  }

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamReaderBase()
  {
  }

  /// \brief Reads a raw number of bytes into the read buffer, this is the only method which has to be implemented to fully implement the interface.
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

};

/// \brief Interface for binary out (write) streams.
class EZ_FOUNDATION_DLL ezStreamWriterBase
{

public:

  /// \brief Constructor
  ezStreamWriterBase()
  {
  }

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~ezStreamWriterBase()
  {
  }

  /// \brief Writes a raw number of bytes from the buffer, this is the only method which has to be implemented to fully implement the interface.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) = 0; // [tested]

  /// \brief Flushes the stream, may be implemented (not necessary to implement the interface correctly) so that user code can ensure that content is written
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

};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>
#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>
#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>

