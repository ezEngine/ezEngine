
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/IBinaryStream.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Basics/Types/RefCounted.h>

class ezMemoryStreamReader;
class ezMemoryStreamWriter;

/// Instances of this class act as storage for memory streams
class EZ_FOUNDATION_DLL ezMemoryStreamStorage : public ezIRefCounted
{
public:

  ezMemoryStreamStorage(ezUInt32 uiInitialCapacity = 0, ezIAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  ~ezMemoryStreamStorage();

private:

  friend class ezMemoryStreamReader;

  friend class ezMemoryStreamWriter;

  ezDynamicArray<ezUInt8> m_Storage;

};

/// A reader which can access a memory stream
/// Please note that the functions exposed by this object are not thread safe! If access to the same ezMemoryStreamStorage object from
/// multiple threads is desired please create one instance of ezMemoryStreamReader per thread.
class EZ_FOUNDATION_DLL ezMemoryStreamReader : public ezIBinaryStreamReader
{
public:

  ezMemoryStreamReader(ezMemoryStreamStorage* pStreamStorage);

  ~ezMemoryStreamReader();

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  /// It is valid to pass NULL for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) EZ_OVERRIDE;

  /// Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip);

  /// TODO: Not sure, if this functionality is needed, at all.
  ///  However, if yes, I would probably just implement a "SetReadPosition" function, instead of this very limited function.
  /// For that it would probably also make sense to have a "GetByteCount" function, or so.
  void Rewind();

private:

  friend class ezMemoryStreamStorage;

  ezScopedRefPointer<ezMemoryStreamStorage> m_pStreamStorage;

  ezUInt32 m_uiReadPosition;
};

/// A writer which can access a memory stream
/// Please note that the functions exposed by this object are not thread safe!
class EZ_FOUNDATION_DLL ezMemoryStreamWriter : public ezIBinaryStreamWriter
{
public:

  ezMemoryStreamWriter(ezMemoryStreamStorage* pStreamStorage);

  ~ezMemoryStreamWriter();

  /// Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) EZ_OVERRIDE;

  /// TODO: If we want to have such functionality, a "SetWritePosition" would be much more useful.
  void Rewind();

private:

  friend class ezMemoryStreamStorage;

  ezScopedRefPointer<ezMemoryStreamStorage> m_pStreamStorage;

  ezUInt32 m_uiWritePosition;
};