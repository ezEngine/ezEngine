#include <FoundationPCH.h>

#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

ezDataDirectory::ArchiveType::ArchiveType() = default;
ezDataDirectory::ArchiveType::~ArchiveType() = default;

ezDataDirectoryType* ezDataDirectory::ArchiveType::Factory(
  const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage)
{
  ArchiveType* pDataDir = EZ_DEFAULT_NEW(ArchiveType);

  if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
    return pDataDir;

  EZ_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

ezDataDirectoryReader* ezDataDirectory::ArchiveType::OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir)
{
  const ezArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  const ezUInt32 uiEntryIndex = toc.FindEntry(szFile);

  if (uiEntryIndex == ezInvalidIndex)
    return nullptr;

  const ezArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  // TODO: reuse readers
  ArchiveReaderUncompressed* pReader = nullptr;

  switch (pEntry->m_CompressionMode)
  {
  case ezArchiveCompressionMode::Uncompressed:
      pReader = EZ_DEFAULT_NEW(ArchiveReaderUncompressed);
      break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  case ezArchiveCompressionMode::Compressed_zstd:
      pReader = EZ_DEFAULT_NEW(ArchiveReaderZstd);
      break;
#endif

    default:
      EZ_REPORT_FAILURE("Compression mode {} is unknown (or not compiled in)", (ezUInt8)pEntry->m_CompressionMode);
      return nullptr;
  }

  pReader->m_uiUncompressedSize = pEntry->m_uiUncompressedDataSize;

  m_ArchiveReader.ConfigureRawMemoryStreamReader(uiEntryIndex, pReader->m_MemStreamReader);

  if (pReader->Open(szFile, this).Failed())
  {
    EZ_DEFAULT_DELETE(pReader);
    return nullptr;
  }

  return pReader;
}

void ezDataDirectory::ArchiveType::RemoveDataDirectory()
{
  ArchiveType* pThis = this;
  EZ_DEFAULT_DELETE(pThis);
}

bool ezDataDirectory::ArchiveType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
{
  return m_ArchiveReader.GetArchiveTOC().FindEntry(szFile) != ezInvalidIndex;
}

ezResult ezDataDirectory::ArchiveType::GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
{
  const ezArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  const ezUInt32 uiEntryIndex = toc.FindEntry(szFileOrFolder);

  if (uiEntryIndex == ezInvalidIndex)
    return EZ_FAILURE;

  const ezArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  const char* szPath = toc.GetEntryPathString(uiEntryIndex);

  out_Stats.m_bIsDirectory = false;
  out_Stats.m_LastModificationTime = m_LastModificationTime;
  out_Stats.m_uiFileSize = pEntry->m_uiUncompressedDataSize;
  out_Stats.m_sParentPath = szPath;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = ezPathUtils::GetFileNameAndExtension(szPath);

  return EZ_SUCCESS;
}

ezResult ezDataDirectory::ArchiveType::InternalInitializeDataDirectory(const char* szDirectory)
{
  ezStringBuilder sRedirected;
  if (ezFileSystem::ResolveSpecialDirectory(szDirectory, sRedirected).Succeeded())
  {
    m_sRedirectedDataDirPath = sRedirected;
  }
  else
  {
    m_sRedirectedDataDirPath = szDirectory;
  }

  // remove trailing slashes
  sRedirected.Trim("/\\");

  if (!sRedirected.HasExtension("ezArchive"))
    return EZ_FAILURE;

  ezFileStats stats;
  if (ezOSFile::GetFileStats(sRedirected, stats).Failed())
    return EZ_FAILURE;

  EZ_LOG_BLOCK("ezArchiveDataDir", szDirectory);

  m_LastModificationTime = stats.m_LastModificationTime;

  EZ_SUCCEED_OR_RETURN(m_ArchiveReader.OpenArchive(sRedirected));

  ReloadExternalConfigs();

  return EZ_SUCCESS;
}

void ezDataDirectory::ArchiveType::OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed)
{
  // TODO: reuse readers
  EZ_DEFAULT_DELETE(pClosed);
}

//////////////////////////////////////////////////////////////////////////

ezDataDirectory::ArchiveReaderUncompressed::ArchiveReaderUncompressed() = default;
ezDataDirectory::ArchiveReaderUncompressed::~ArchiveReaderUncompressed() = default;

ezUInt64 ezDataDirectory::ArchiveReaderUncompressed::Read(void* pBuffer, ezUInt64 uiBytes)
{
  return m_MemStreamReader.ReadBytes(pBuffer, uiBytes);
}

ezUInt64 ezDataDirectory::ArchiveReaderUncompressed::GetFileSize() const
{
  return m_uiUncompressedSize;
}

ezResult ezDataDirectory::ArchiveReaderUncompressed::InternalOpen()
{
  // nothing to do
  return EZ_SUCCESS;
}

void ezDataDirectory::ArchiveReaderUncompressed::InternalClose()
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////

ezDataDirectory::ArchiveReaderZstd::ArchiveReaderZstd() = default;
ezDataDirectory::ArchiveReaderZstd::~ArchiveReaderZstd() = default;

ezUInt64 ezDataDirectory::ArchiveReaderZstd::Read(void* pBuffer, ezUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

ezResult ezDataDirectory::ArchiveReaderZstd::InternalOpen()
{
  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader);
  return EZ_SUCCESS;
}
