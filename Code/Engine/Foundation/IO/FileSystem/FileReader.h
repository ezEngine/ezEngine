#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/Implementation/FileReaderWriterBase.h>
#include <Foundation/IO/Stream.h>

/// \brief The default class to use to read data from a file, implements the ezStreamReader interface.
///
/// This file reader buffers reads up to a certain amount of bytes (configurable).
/// It closes the file automatically once it goes out of scope.
class EZ_FOUNDATION_DLL ezFileReader : public ezFileReaderBase
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezFileReader);

public:
  /// \brief Constructor, does nothing.
  ezFileReader()

    = default;

  /// \brief Destructor, closes the file, if it is still open (RAII).
  ~ezFileReader() { Close(); }

  /// \brief Opens the given file for reading. Returns EZ_SUCCESS if the file could be opened. A cache is created to speed up small reads.
  ///
  /// You should typically not disable bAllowFileEvents, unless you need to prevent recursive file events,
  /// which is only the case, if you are doing file accesses from within a File Event Handler.
  ezResult Open(ezStringView sFile, ezUInt32 uiCacheSize = 1024 * 64, ezFileShareMode::Enum fileShareMode = ezFileShareMode::Default, bool bAllowFileEvents = true);

  /// \brief Closes the file, if it is open.
  void Close();

  /// \brief Attempts to read the given number of bytes into the buffer. Returns the actual number of bytes read.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override;

  /// \brief Helper method to skip a number of bytes. Returns the actual number of bytes skipped.
  virtual ezUInt64 SkipBytes(ezUInt64 uiBytesToSkip) override;
  /// \brief Whether the end of the file was reached during reading.
  ///
  /// \note This is not 100% accurate, it does not guarantee that if it returns false, that the next read will return any data.
  bool IsEOF() const { return m_bEOF; }

private:
  ezUInt64 m_uiBytesCached = 0;
  ezUInt64 m_uiCacheReadPosition = 0;
  ezDynamicArray<ezUInt8> m_Cache;
  bool m_bEOF = true;
};
