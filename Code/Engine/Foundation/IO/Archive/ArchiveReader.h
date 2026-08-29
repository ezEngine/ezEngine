#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Types/UniquePtr.h>

class ezRawMemoryStreamReader;
class ezStreamReader;

/// A utility class for reading from ezArchive files
class EZ_FOUNDATION_DLL ezArchiveReader
{
public:
  /// Opens the given file and validates that it is a valid archive file.
  ezResult OpenArchive(ezStringView sPath);

  /// Returns the table-of-contents for the previously opened archive.
  const ezArchiveTOC& GetArchiveTOC();

  /// Extracts the given entry to the target folder.
  ///
  /// Calls ExtractFileProgressCallback() to report progress.
  ezResult ExtractFile(ezUInt32 uiEntryIdx, ezStringView sTargetFolder) const;

  /// Extracts all files to the target folder.
  ///
  /// Calls ExtractNextFileCallback() for every file that is being extracted.
  ezResult ExtractAllFiles(ezStringView sTargetFolder) const;

  /// Sets up \a memReader for reading the raw (potentially compressed) data that is stored for the given entry in the archive.
  void ConfigureRawMemoryStreamReader(ezUInt32 uiEntryIdx, ezRawMemoryStreamReader& ref_memReader) const;

  /// Creates a reader that will decompress the given file entry.
  ezUniquePtr<ezStreamReader> CreateEntryReader(ezUInt32 uiEntryIdx) const;

protected:
  /// Called by ExtractAllFiles() for progress reporting. Return false to abort.
  virtual bool ExtractNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile) const;

  /// Called by ExtractFile() for progress reporting. Return false to abort.
  virtual bool ExtractFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const;

  ezMemoryMappedFile m_MemFile;
  ezArchiveTOC m_ArchiveTOC;
  ezUInt8 m_uiArchiveVersion = 0;
  const void* m_pDataStart = nullptr;
  ezUInt64 m_uiMemFileSize = 0;
};
