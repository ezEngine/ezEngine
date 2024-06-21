#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

void ezArchiveBuilder::AddFolder(ezStringView sAbsFolderPath, ezArchiveCompressionMode defaultMode /*= ezArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezFileSystemIterator fileIt;

  ezStringBuilder sBasePath = sAbsFolderPath;
  sBasePath.MakeCleanPath();

  ezStringBuilder fullPath;
  ezStringBuilder relPath;

  for (fileIt.StartSearch(sBasePath, ezFileSystemIteratorFlags::ReportFilesRecursive); fileIt.IsValid(); fileIt.Next())
  {
    const auto& stat = fileIt.GetStats();

    stat.GetFullPath(fullPath);
    relPath = fullPath;

    if (relPath.MakeRelativeTo(sBasePath).Succeeded())
    {
      ezArchiveCompressionMode compression = defaultMode;
      ezInt32 iCompressionLevel = 0;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = ezArchiveCompressionMode::Uncompressed;
            break;

#  ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
          case InclusionMode::Compress_zstd_fastest:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<ezInt32>(ezCompressedStreamWriterZstd::Compression::Fastest);
            break;
          case InclusionMode::Compress_zstd_fast:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<ezInt32>(ezCompressedStreamWriterZstd::Compression::Fast);
            break;
          case InclusionMode::Compress_zstd_average:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<ezInt32>(ezCompressedStreamWriterZstd::Compression::Average);
            break;
          case InclusionMode::Compress_zstd_high:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<ezInt32>(ezCompressedStreamWriterZstd::Compression::High);
            break;
          case InclusionMode::Compress_zstd_highest:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<ezInt32>(ezCompressedStreamWriterZstd::Compression::Highest);
            break;
#  endif
        }
      }

      auto& e = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath = fullPath;
      e.m_sRelTargetPath = relPath;
      e.m_CompressionMode = compression;
      e.m_iCompressionLevel = iCompressionLevel;
    }
  }

#else
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif
}

ezResult ezArchiveBuilder::WriteArchive(ezStringView sFile) const
{
  EZ_LOG_BLOCK("WriteArchive", sFile);

  ezFileWriter file;
  if (file.Open(sFile, 1024 * 1024 * 16).Failed())
  {
    ezLog::Error("Could not open file for writing archive to: '{}'", sFile);
    return EZ_FAILURE;
  }

  return WriteArchive(file);
}

ezResult ezArchiveBuilder::WriteArchive(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(ezArchiveUtils::WriteHeader(inout_stream));

  ezArchiveTOC toc;

  ezStringBuilder sHashablePath;

  ezUInt64 uiStreamSize = 0;
  const ezUInt32 uiNumEntries = m_Entries.GetCount();

  ezStopwatch sw;

  for (ezUInt32 i = 0; i < uiNumEntries; ++i)
  {
    const SourceEntry& e = m_Entries[i];

    const ezUInt32 uiPathStringOffset = toc.AddPathString(e.m_sRelTargetPath);

    sHashablePath = e.m_sRelTargetPath;
    sHashablePath.ToLower();

    toc.m_PathToEntryIndex[ezArchiveStoredString(ezHashingUtils::StringHash(sHashablePath), uiPathStringOffset)] = toc.m_Entries.GetCount();

    if (!WriteNextFileCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath))
      return EZ_FAILURE;

    ezArchiveEntry& tocEntry = toc.m_Entries.ExpandAndGetRef();

    EZ_SUCCEED_OR_RETURN(ezArchiveUtils::WriteEntryOptimal(inout_stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode, e.m_iCompressionLevel, tocEntry, uiStreamSize, ezMakeDelegate(&ezArchiveBuilder::WriteFileProgressCallback, this)));

    WriteFileResultCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath, tocEntry.m_uiUncompressedDataSize, tocEntry.m_uiStoredDataSize, sw.Checkpoint());
  }

  EZ_SUCCEED_OR_RETURN(ezArchiveUtils::AppendTOC(inout_stream, toc));

  return EZ_SUCCESS;
}

bool ezArchiveBuilder::WriteNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile) const
{
  EZ_IGNORE_UNUSED(uiCurEntry);
  EZ_IGNORE_UNUSED(uiMaxEntries);
  EZ_IGNORE_UNUSED(sSourceFile);
  return true;
}

bool ezArchiveBuilder::WriteFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const
{
  EZ_IGNORE_UNUSED(bytesWritten);
  EZ_IGNORE_UNUSED(bytesTotal);
  return true;
}
