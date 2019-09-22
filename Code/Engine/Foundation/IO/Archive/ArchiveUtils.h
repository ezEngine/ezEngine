#pragma once

#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/IO/Archive/Archive.h>

class ezStreamReader;
class ezStreamWriter;
class ezMemoryMappedFile;
class ezArchiveTOC;
class ezArchiveEntry;
class ezRawMemoryStreamReader;

/// \brief Utilities for working with ezArchive files
namespace ezArchiveUtils
{
  typedef ezDelegate<bool(ezUInt64, ezUInt64)> FileWriteProgressCallback;

  /// \brief Writes the header that identifies the ezArchive file and version to the stream
  EZ_FOUNDATION_DLL ezResult WriteHeader(ezStreamWriter& stream);

  /// \brief Reads the ezArchive header. Returns success and the version, if the stream is a valid ezArchive file.
  EZ_FOUNDATION_DLL ezResult ReadHeader(ezStreamReader& stream, ezUInt8& out_uiVersion);

  /// \brief Writes the archive TOC to the stream. This must be the last thing in the stream, if ExtractTOC() is supposed to work.
  EZ_FOUNDATION_DLL ezResult AppendTOC(ezStreamWriter& stream, const ezArchiveTOC& toc);

  /// \brief Deserializes the TOC from the memory mapped file. Assumes the TOC is the very last data in the file and reads it from the back.
  EZ_FOUNDATION_DLL ezResult ExtractTOC(ezMemoryMappedFile& memFile, ezArchiveTOC& toc);

  /// \brief Writes a single file entry to an ezArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. The progress callback is executed for every couple of KB of data that were written.
  EZ_FOUNDATION_DLL ezResult WriteEntry(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset,
    ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Similar to WriteEntry, but if compression is enabled, checks that compression makes enough of a difference.
  /// If compression does not reduce file size enough, the file is stored uncompressed instead.
  EZ_FOUNDATION_DLL ezResult WriteEntryOptimal(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset,
    ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Configures \a memReader as a view into the data stored for \a entry in the archive file.
  ///
  /// The raw memory stream may be compressed or uncompressed. This only creates a view for the stored data, it does not interpret it.
  EZ_FOUNDATION_DLL void ConfigureRawMemoryStreamReader(
    const ezArchiveEntry& entry, const void* pStartOfArchiveData, ezRawMemoryStreamReader& memReader);

  /// \brief Creates a new stream reader which allows to read the uncompressed data for the given archive entry.
  ///
  /// Under the hood it may create different types of stream readers to uncompress or decode the data.
  EZ_FOUNDATION_DLL ezUniquePtr<ezStreamReader> CreateEntryReader(const ezArchiveEntry& entry, const void* pStartOfArchiveData);

  EZ_FOUNDATION_DLL ezResult ReadZipHeader(ezStreamReader& stream, ezUInt8& out_uiVersion);
  EZ_FOUNDATION_DLL ezResult ExtractZipTOC(ezMemoryMappedFile& memFile, ezArchiveTOC& toc);


} // namespace ezArchiveUtils
