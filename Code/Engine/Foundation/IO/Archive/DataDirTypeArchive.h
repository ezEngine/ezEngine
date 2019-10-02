#pragma once

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/IO/MemoryStream.h>

class ezArchiveEntry;

namespace ezDataDirectory
{
  class ArchiveReaderUncompressed;
  class ArchiveReaderZstd;
  class ArchiveReaderZip;

  class EZ_FOUNDATION_DLL ArchiveType : public ezDataDirectoryType
  {
  public:
    ArchiveType();
    ~ArchiveType();

    static ezDataDirectoryType* Factory(
      const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage);

    virtual const ezString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile, ezFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual void RemoveDataDirectory() override;

    virtual bool ExistsFile(const char* szFile, bool bOneSpecificDataDir) override;

    virtual ezResult GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) override;

    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;

    virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) override;

    ezString128 m_sRedirectedDataDirPath;
    ezString32 m_sArchiveSubFolder;
    ezTimestamp m_LastModificationTime;
    ezArchiveReader m_ArchiveReader;

    ezMutex m_ReaderMutex;
    ezHybridArray<ezUniquePtr<ArchiveReaderUncompressed>, 4> m_ReadersUncompressed;
    ezHybridArray<ArchiveReaderUncompressed*, 4> m_FreeReadersUncompressed;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    ezHybridArray<ezUniquePtr<ArchiveReaderZstd>, 4> m_ReadersZstd;
    ezHybridArray<ArchiveReaderZstd*, 4> m_FreeReadersZstd;
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    ezHybridArray<ezUniquePtr<ArchiveReaderZip>, 4> m_ReadersZip;
    ezHybridArray<ArchiveReaderZip*, 4> m_FreeReadersZip;
#endif
  };

  class EZ_FOUNDATION_DLL ArchiveReaderUncompressed : public ezDataDirectoryReader
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderUncompressed);

  public:
    ArchiveReaderUncompressed(ezInt32 iDataDirUserData);
    ~ArchiveReaderUncompressed();

    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;
    virtual ezUInt64 GetFileSize() const override;

  protected:
    virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class ArchiveType;

    ezUInt64 m_uiUncompressedSize = 0;
    ezUInt64 m_uiCompressedSize = 0;
    ezRawMemoryStreamReader m_MemStreamReader;
  };

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  class EZ_FOUNDATION_DLL ArchiveReaderZstd : public ArchiveReaderUncompressed
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZstd);

  public:
    ArchiveReaderZstd(ezInt32 iDataDirUserData);
    ~ArchiveReaderZstd();

    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;

  protected:
    virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;

    friend class ArchiveType;

    ezCompressedStreamReaderZstd m_CompressedStreamReader;
  };
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  class EZ_FOUNDATION_DLL ArchiveReaderZip : public ArchiveReaderUncompressed
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZip);

  public:
    ArchiveReaderZip(ezInt32 iDataDirUserData);
    ~ArchiveReaderZip();

    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;

  protected:
    virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;

    friend class ArchiveType;

    ezCompressedStreamReaderZip m_CompressedStreamReader;
  };
#endif
} // namespace ezDataDirectory
