#include <FoundationPCH.h>

#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ArchiveDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem", "FolderDataDirectory"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::ArchiveType::Factory);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

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
  ezStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(szFile);

  const ezUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == ezInvalidIndex)
    return nullptr;

  const ezArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  ArchiveReaderUncompressed* pReader = nullptr;

  {
    EZ_LOCK(m_ReaderMutex);

    switch (pEntry->m_CompressionMode)
    {
      case ezArchiveCompressionMode::Uncompressed:
      {
        if (!m_FreeReadersUncompressed.IsEmpty())
        {
          pReader = m_FreeReadersUncompressed.PeekBack();
          m_FreeReadersUncompressed.PopBack();
        }
        else
        {
          m_ReadersUncompressed.PushBack(EZ_DEFAULT_NEW(ArchiveReaderUncompressed, 0));
          pReader = m_ReadersUncompressed.PeekBack().Borrow();
        }
        break;
      }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      case ezArchiveCompressionMode::Compressed_zstd:
      {
        if (!m_FreeReadersZstd.IsEmpty())
        {
          pReader = m_FreeReadersZstd.PeekBack();
          m_FreeReadersZstd.PopBack();
        }
        else
        {
          m_ReadersZstd.PushBack(EZ_DEFAULT_NEW(ArchiveReaderZstd, 1));
          pReader = m_ReadersZstd.PeekBack().Borrow();
        }
        break;
      }
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
      case ezArchiveCompressionMode::Compressed_zip:
      {
        if (!m_FreeReadersZip.IsEmpty())
        {
          pReader = m_FreeReadersZip.PeekBack();
          m_FreeReadersZip.PopBack();
        }
        else
        {
          m_ReadersZip.PushBack(EZ_DEFAULT_NEW(ArchiveReaderZip, 2));
          pReader = m_ReadersZip.PeekBack().Borrow();
        }
        break;
      }
#endif

      default:
        EZ_REPORT_FAILURE("Compression mode {} is unknown (or not compiled in)", (ezUInt8)pEntry->m_CompressionMode);
        return nullptr;
    }
  }

  pReader->m_uiUncompressedSize = pEntry->m_uiUncompressedDataSize;
  pReader->m_uiCompressedSize = pEntry->m_uiStoredDataSize;

  m_ArchiveReader.ConfigureRawMemoryStreamReader(uiEntryIndex, pReader->m_MemStreamReader);

  if (pReader->Open(sArchivePath, this).Failed())
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
  ezStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(szFile);
  return m_ArchiveReader.GetArchiveTOC().FindEntry(sArchivePath) != ezInvalidIndex;
}

ezResult ezDataDirectory::ArchiveType::GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
{
  const ezArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  ezStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(szFileOrFolder);
  const ezUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

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
  ezFileSystem::ResolveSpecialDirectory(szDirectory, sRedirected);
  sRedirected.MakeCleanPath();
  // remove trailing slashes
  sRedirected.Trim("", "/");
  m_sRedirectedDataDirPath = sRedirected;

  
  bool bSupported = false;
  ezStringBuilder sArchivePath;
  const char* szExtensions[] = {"ezArchive",
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    "zip", "apk",
#endif
  };
  ezArrayPtr<const char*> extensions(szExtensions);
  for (auto ext : extensions)
  {
    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(ext);
    if (sRedirected.HasExtension(ext))
    {
      sArchivePath = sRedirected;
      m_sArchiveSubFolder = "";
      bSupported = true;
      goto endloop;
    }
    const char* szFound = nullptr;
    do
    {
      szFound = sRedirected.FindLastSubString_NoCase(ext, szFound);
      if (szFound != nullptr && szFound[uiLength] == '/')
      {
        sArchivePath = ezStringView(sRedirected.GetData(), szFound + uiLength);
        m_sArchiveSubFolder = szFound + uiLength + 1;
        bSupported = true;
        goto endloop;
      }

    } while (szFound != nullptr);

  }
endloop:
  if (!bSupported)
    return EZ_FAILURE;

  ezFileStats stats;
  if (ezOSFile::GetFileStats(sArchivePath, stats).Failed())
    return EZ_FAILURE;

  EZ_LOG_BLOCK("ezArchiveDataDir", szDirectory);

  m_LastModificationTime = stats.m_LastModificationTime;

  EZ_SUCCEED_OR_RETURN(m_ArchiveReader.OpenArchive(sArchivePath));

  ReloadExternalConfigs();

  return EZ_SUCCESS;
}

void ezDataDirectory::ArchiveType::OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed)
{
  EZ_LOCK(m_ReaderMutex);

  if (pClosed->GetDataDirUserData() == 0)
  {
    m_FreeReadersUncompressed.PushBack(static_cast<ArchiveReaderUncompressed*>(pClosed));
    return;
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (pClosed->GetDataDirUserData() == 1)
  {
    m_FreeReadersZstd.PushBack(static_cast<ArchiveReaderZstd*>(pClosed));
    return;
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  if (pClosed->GetDataDirUserData() == 2)
  {
    m_FreeReadersZip.PushBack(static_cast<ArchiveReaderZip*>(pClosed));
    return;
  }
#endif

  EZ_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

ezDataDirectory::ArchiveReaderUncompressed::ArchiveReaderUncompressed(ezInt32 iDataDirUserData)
  : ezDataDirectoryReader(iDataDirUserData)
{
}

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

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

ezDataDirectory::ArchiveReaderZstd::ArchiveReaderZstd(ezInt32 iDataDirUserData)
  : ArchiveReaderUncompressed(iDataDirUserData)
{
}

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

#endif

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

ezDataDirectory::ArchiveReaderZip::ArchiveReaderZip(ezInt32 iDataDirUserData)
  : ArchiveReaderUncompressed(iDataDirUserData)
{
}

ezDataDirectory::ArchiveReaderZip::~ArchiveReaderZip() = default;

ezUInt64 ezDataDirectory::ArchiveReaderZip::Read(void* pBuffer, ezUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

ezResult ezDataDirectory::ArchiveReaderZip::InternalOpen()
{
  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader, m_uiCompressedSize);
  return EZ_SUCCESS;
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_DataDirTypeArchive);

