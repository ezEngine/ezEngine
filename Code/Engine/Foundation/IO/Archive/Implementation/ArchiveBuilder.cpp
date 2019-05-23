#include <FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) || defined(EZ_DOCS)
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void ezArchiveBuilder::AddFolder(const char* szAbsFolderPath,
  ezArchiveCompressionMode defaultMode /*= ezArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
  ezFileSystemIterator fileIt;

  ezStringBuilder sBasePath = szAbsFolderPath;
  sBasePath.MakeCleanPath();

  if (fileIt.StartSearch(sBasePath, true, false).Failed())
    return;

  ezStringBuilder fullPath;
  ezStringBuilder relPath;

  do
  {
    const auto& stat = fileIt.GetStats();

    stat.GetFullPath(fullPath);
    relPath = fullPath;

    if (relPath.MakeRelativeTo(sBasePath).Succeeded())
    {
      ezArchiveCompressionMode compression = defaultMode;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = ezArchiveCompressionMode::Uncompressed;
            break;

          case InclusionMode::Compress_zstd:
            compression = ezArchiveCompressionMode::Compressed_zstd;
            break;
        }
      }

      auto& e = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath = fullPath;
      e.m_sRelTargetPath = relPath;
      e.m_CompressionMode = compression;
    }

  } while (fileIt.Next().Succeeded());
}

ezResult ezArchiveBuilder::WriteArchive(const char* szFile) const
{
  EZ_LOG_BLOCK("WriteArchive", szFile);

  ezFileWriter file;
  if (file.Open(szFile, 1024 * 1024 * 16).Failed())
  {
    ezLog::Error("Could not open file for writing archive to: '{}'", szFile);
    return EZ_FAILURE;
  }

  return WriteArchive(file);
}

ezResult ezArchiveBuilder::WriteArchive(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(ezArchiveUtils::WriteHeader(stream));

  ezArchiveTOC toc;

  ezStringBuilder sHashablePath;

  ezUInt64 uiStreamSize = 0;
  const ezUInt32 uiNumEntries = m_Entries.GetCount();

  for (ezUInt32 i = 0; i < uiNumEntries; ++i)
  {
    const SourceEntry& e = m_Entries[i];

    const ezUInt32 uiPathStringOffset = toc.m_AllPathStrings.GetCount();
    toc.m_AllPathStrings.PushBackRange(
      ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(e.m_sRelTargetPath.GetData()), e.m_sRelTargetPath.GetElementCount() + 1));

    sHashablePath = e.m_sRelTargetPath;
    sHashablePath.ToLower();

    toc.m_PathToIndex[ezTempHashedString(sHashablePath.GetData())] = toc.m_Entries.GetCount();

    if (!WriteNextFileCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath))
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(ezArchiveUtils::WriteEntryOptimal(stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode,
      toc.m_Entries.ExpandAndGetRef(), uiStreamSize, ezMakeDelegate(&ezArchiveBuilder::WriteFileProgressCallback, this)));
  }

  EZ_SUCCEED_OR_RETURN(ezArchiveUtils::AppendTOC(stream, toc));

  return EZ_SUCCESS;
}

bool ezArchiveBuilder::WriteNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, const char* szSourceFile) const
{
  return true;
}

bool ezArchiveBuilder::WriteFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const
{
  return true;
}
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveBuilder);

