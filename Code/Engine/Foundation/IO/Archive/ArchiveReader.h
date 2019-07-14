#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/IO/MemoryMappedFile.h>

class ezRawMemoryStreamReader;
class ezStreamReader;

/// \brief A utility class for reading from ezArchive files
class EZ_FOUNDATION_DLL ezArchiveReader
{
public:
  /// \brief Opens the given file and validates that it is a valid archive file.
  ezResult OpenArchive(const char* szPath);

  /// \brief Returns the table-of-contents for the previously opened archive.
  const ezArchiveTOC& GetArchiveTOC();

  /// \brief Extracts the given entry to the target folder.
  ///
  /// Calls ExtractFileProgressCallback() to report progress.
  ezResult ExtractFile(ezUInt32 uiEntryIdx, const char* szTargetFolder) const;

  /// \brief Extracts all files to the target folder.
  ///
  /// Calls ExtractNextFileCallback() for every file that is being extracted.
  ezResult ExtractAllFiles(const char* szTargetFolder) const;

  /// \brief Sets up \a memReader for reading the raw (potentially compressed) data that is stored for the given entry in the archive.
  void ConfigureRawMemoryStreamReader(ezUInt32 uiEntryIdx, ezRawMemoryStreamReader& memReader) const;

  /// \brief Creates a reader that will decompress the given file entry.
  ezUniquePtr<ezStreamReader> CreateEntryReader(ezUInt32 uiEntryIdx) const;

protected:
  /// \brief Called by ExtractAllFiles() for progress reporting. Return false to abort.
  virtual bool ExtractNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, const char* szSourceFile) const;

  /// \brief Called by ExtractFile() for progress reporting. Return false to abort.
  virtual bool ExtractFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const;

  ezMemoryMappedFile m_MemFile;
  ezArchiveTOC m_ArchiveTOC;
  ezUInt8 m_uiArchiveVersion = 0;
  const void* m_pDataStart = nullptr;
};
