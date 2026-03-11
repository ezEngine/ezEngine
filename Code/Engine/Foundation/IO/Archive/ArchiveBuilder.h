#pragma once

#include <Foundation/IO/Archive/Archive.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/Delegate.h>

/// \brief Utility class to build an ezArchive file from files/folders on disk
///
/// All functionality for writing an ezArchive file is available through ezArchiveUtils.
class EZ_FOUNDATION_DLL ezArchiveBuilder
{
public:
  struct SourceEntry
  {
    ezString m_sAbsSourcePath; ///< The source file to read
    ezString m_sRelTargetPath; ///< Under which relative path to store it in the ezArchive
    ezArchiveCompressionMode m_CompressionMode = ezArchiveCompressionMode::Uncompressed;
    ezInt32 m_iCompressionLevel = 0;
  };

  // all the source files from disk that should be put into the ezArchive
  ezDeque<SourceEntry> m_Entries;

  enum class InclusionMode
  {
    Exclude,               ///< Do not add this file to the archive
    Uncompressed,          ///< Add the file to the archive, but do not even try to compress it
    Compress_zstd_fastest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_fast,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_average, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_high,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_highest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
  };

  /// \brief Custom decider whether to include a file into the archive
  using InclusionCallback = ezDelegate<InclusionMode(ezStringView)>;

  /// \brief Iterates over all files in a folder and adds them to m_Entries for later.
  ///
  /// The callback can be used to exclude certain files or to deactivate compression on them.
  /// \note If no callback is given, the default is to store all files uncompressed!
  void AddFolder(ezStringView sAbsFolderPath, ezArchiveCompressionMode defaultMode = ezArchiveCompressionMode::Uncompressed, InclusionCallback callback = InclusionCallback());

  /// \brief Overwrites the given file with the archive
  ezResult WriteArchive(ezStringView sFile) const;

  /// \brief Writes the previously gathered files to the file stream
  ezResult WriteArchive(ezStreamWriter& inout_stream) const;

protected:
  /// Override this to get a callback when the next file is being written to the output. Return 'true' to continue, 'false' to cancel the entire archive generation.
  virtual bool WriteNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile) const;

  /// Override this to get a progress report for writing a single file to the output
  virtual bool WriteFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const;

  /// Override this to get a callback after a file has been processed. Gets additional information about the compression result and duration.
  virtual void WriteFileResultCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile, ezUInt64 uiSourceSize, ezUInt64 uiStoredSize, ezTime duration) const
  {
    EZ_IGNORE_UNUSED(uiCurEntry);
    EZ_IGNORE_UNUSED(uiMaxEntries);
    EZ_IGNORE_UNUSED(sSourceFile);
    EZ_IGNORE_UNUSED(uiSourceSize);
    EZ_IGNORE_UNUSED(uiStoredSize);
    EZ_IGNORE_UNUSED(duration);
  }
};
