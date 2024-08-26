#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class ezStreamReader;
class ezStreamWriter;
class ezMemoryMappedFile;
class ezArchiveTOC;
class ezArchiveEntry;
class ezRawMemoryStreamReader;

/// \brief Utilities for working with ezArchive files
namespace ezArchiveUtils
{
  using FileWriteProgressCallback = ezDelegate<bool(ezUInt64, ezUInt64)>;
  constexpr ezUInt32 ArchiveHeaderSize = 16;
  constexpr ezUInt32 ArchiveTOCMetaMaxFooterSize = 14 + 12; //< note that it's the MAX size, i.e. toc meta can be smaller

  struct TOCMeta
  {
    ezUInt32 m_uiTocSize = 0;
    ezUInt64 m_uiExpectedTocHash = 0;
    ezUInt32 m_uiTocOffsetFromArchiveEnd = 0;
  };

  /// \brief Returns a modifiable array of file extensions that the engine considers to be valid ezArchive file extensions.
  ///
  /// By default it always contains 'ezArchive'.
  /// Add or overwrite the values, if you want custom file extensions to be handled as ezArchives.
  EZ_FOUNDATION_DLL ezHybridArray<ezString, 4, ezStaticsAllocatorWrapper>& GetAcceptedArchiveFileExtensions();

  /// \brief Checks case insensitive, whether the given extension is in the list of GetAcceptedArchiveFileExtensions().
  EZ_FOUNDATION_DLL bool IsAcceptedArchiveFileExtensions(ezStringView sExtension);

  /// \brief Writes the header that identifies the ezArchive file and version to the stream
  EZ_FOUNDATION_DLL ezResult WriteHeader(ezStreamWriter& inout_stream);

  /// \brief Reads the ezArchive header. Returns success and the version, if the stream is a valid ezArchive file.
  EZ_FOUNDATION_DLL ezResult ReadHeader(ezStreamReader& inout_stream, ezUInt8& out_uiVersion);

  /// \brief Writes the archive TOC to the stream. This must be the last thing in the stream, if ExtractTOC() is supposed to work.
  EZ_FOUNDATION_DLL ezResult AppendTOC(ezStreamWriter& inout_stream, const ezArchiveTOC& toc);

  /// \brief Deserializes the TOC meta from archive ending. Assumes the TOC is the very last data in the file.
  EZ_FOUNDATION_DLL ezResult ExtractTOCMeta(ezUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, ezUInt8 uiArchiveVersion);

  /// \brief Deserializes the TOC meta from the memory mapped file. Assumes the TOC is the very last data in the file.
  EZ_FOUNDATION_DLL ezResult ExtractTOCMeta(const ezMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, ezUInt8 uiArchiveVersion);

  /// \brief Deserializes the TOC from from archive ending. Assumes the TOC is the very last data in the file and reads it from the back.
  EZ_FOUNDATION_DLL ezResult ExtractTOC(ezUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, ezArchiveTOC& ref_toc, ezUInt8 uiArchiveVersion);

  /// \brief Deserializes the TOC from the memory mapped file. Assumes the TOC is the very last data in the file and reads it from the back.
  EZ_FOUNDATION_DLL ezResult ExtractTOC(const ezMemoryMappedFile& memFile, ezArchiveTOC& ref_toc, ezUInt8 uiArchiveVersion);

  /// \brief Writes a single file entry to an ezArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. The progress callback is executed for every couple of KB of data that were written.
  EZ_FOUNDATION_DLL ezResult WriteEntry(ezStreamWriter& inout_stream, ezStringView sAbsSourcePath, ezUInt32 uiPathStringOffset,
    ezArchiveCompressionMode compression, ezInt32 iCompressionLevel, ezArchiveEntry& ref_tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Writes a single file entry to an ezArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. Compression parameter indicate compression that the entry data already have applied.
  EZ_FOUNDATION_DLL ezResult WriteEntryPreprocessed(ezStreamWriter& inout_stream, ezConstByteArrayPtr entryData, ezUInt32 uiPathStringOffset,
    ezArchiveCompressionMode compression, ezUInt32 uiUncompressedEntryDataSize, ezArchiveEntry& ref_tocEntry, ezUInt64& inout_uiCurrentStreamPosition);

  /// \brief Similar to WriteEntry, but if compression is enabled, checks that compression makes enough of a difference.
  /// If compression does not reduce file size enough, the file is stored uncompressed instead.
  EZ_FOUNDATION_DLL ezResult WriteEntryOptimal(ezStreamWriter& inout_stream, ezStringView sAbsSourcePath, ezUInt32 uiPathStringOffset,
    ezArchiveCompressionMode compression, ezInt32 iCompressionLevel, ezArchiveEntry& ref_tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Configures \a memReader as a view into the data stored for \a entry in the archive file.
  ///
  /// The raw memory stream may be compressed or uncompressed. This only creates a view for the stored data, it does not interpret it.
  EZ_FOUNDATION_DLL void ConfigureRawMemoryStreamReader(
    const ezArchiveEntry& entry, const void* pStartOfArchiveData, ezRawMemoryStreamReader& ref_memReader);

  /// \brief Creates a new stream reader which allows to read the uncompressed data for the given archive entry.
  ///
  /// Under the hood it may create different types of stream readers to uncompress or decode the data.
  EZ_FOUNDATION_DLL ezUniquePtr<ezStreamReader> CreateEntryReader(const ezArchiveEntry& entry, const void* pStartOfArchiveData);

  EZ_FOUNDATION_DLL ezResult ReadZipHeader(ezStreamReader& inout_stream, ezUInt8& out_uiVersion);
  EZ_FOUNDATION_DLL ezResult ExtractZipTOC(const ezMemoryMappedFile& memFile, ezArchiveTOC& ref_toc);


} // namespace ezArchiveUtils
