
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

/// Interface for binary in (read) streams
class EZ_FOUNDATION_DLL ezIBinaryStreamReader 
{

public:

  /// Constructor
  ezIBinaryStreamReader()
  {
  }

  /// Virtual destructor to ensure correct cleanup
  virtual ~ezIBinaryStreamReader()
  {
  }

  /// Reads a raw number of bytes into the read buffer, this is the only method which has to be
  /// implemented to fully implement the interface.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) = 0;

  /// Helper method to read a word value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 ReadWordValue(T* pWordValue);

  /// Helper method to read a dword value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 ReadDWordValue(T* pDWordValue);

  /// Helper method to read a qword value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 ReadQWordValue(T* pQWordValue);

  /// Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip)
  {
    ezUInt8 uiTempBuffer[1024];

    ezUInt64 uiBytesSkipped = 0;

    while(uiBytesSkipped < uiBytesToSkip)
    {
      ezUInt64 uiBytesToRead = ezMath::Min<ezUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      ezUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if(uiBytesRead < uiBytesToSkip)
        break;
    }

    return uiBytesSkipped;
  }

};

/// Interface for binary out (write) streams
class EZ_FOUNDATION_DLL ezIBinaryStreamWriter
{

public:

  /// Constructor
  ezIBinaryStreamWriter()
  {
  }

  /// Virtual destructor to ensure correct cleanup
  virtual ~ezIBinaryStreamWriter()
  {
  }

  /// Writes a raw number of bytes from the buffer, this is the only method which has to be
  /// implemented to fully implement the interface.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) = 0;

  /// Flushes the stream, may be implemented (not necessary to implement the interface correctly) so
  /// that user code can ensure that content is written
  virtual ezResult Flush()
  {
    return EZ_SUCCESS;
  }

  /// Helper method to write a word value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 WriteWordValue(const T* pWordValue); 

  /// Helper method to write a dword value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 WriteDWordValue(const T* pDWordValue);

  /// Helper method to write a qword value correctly (copes with potentially different endianess)
  template <typename T> 
  ezUInt64 WriteQWordValue(const T* pQWordValue);

};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/IBinaryStream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/BinaryStreamOperations_inl.h>
#include <Foundation/IO/Implementation/BinaryStreamOperationsMath_inl.h>
