#pragma once

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Timestamp.h>

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

    static ezDataDirectoryType* Factory(ezStringView sDataDirectory, ezStringView sGroup, ezStringView sRootName, ezDataDirUsage usage);

    virtual const ezString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    virtual ezDataDirectoryReader* OpenFileToRead(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual void RemoveDataDirectory() override;

    virtual bool ExistsFile(ezStringView sFile, bool bOneSpecificDataDir) override;

    virtual ezResult GetFileStats(ezStringView sFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) override;

    virtual ezResult InternalInitializeDataDirectory(ezStringView sDirectory) override;

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

  class EZ_FOUNDATION_DLL ArchiveReaderCommon : public ezDataDirectoryReader
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderCommon);

  public:
    ArchiveReaderCommon(ezInt32 iDataDirUserData);

    virtual ezUInt64 GetFileSize() const override;

  protected:
    friend class ArchiveType;

    ezUInt64 m_uiUncompressedSize = 0;
    ezUInt64 m_uiCompressedSize = 0;
    ezRawMemoryStreamReader m_MemStreamReader;
  };

  class EZ_FOUNDATION_DLL ArchiveReaderUncompressed : public ArchiveReaderCommon
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderUncompressed);

  public:
    ArchiveReaderUncompressed(ezInt32 iDataDirUserData);

    virtual ezUInt64 Skip(ezUInt64 uiBytes) override;
    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;

  protected:
    virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;
  };

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  class EZ_FOUNDATION_DLL ArchiveReaderZstd : public ArchiveReaderCommon
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZstd);

  public:
    ArchiveReaderZstd(ezInt32 iDataDirUserData);

    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;

  protected:
    virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    ezCompressedStreamReaderZstd m_CompressedStreamReader;
  };
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  /// \brief Allows reading of zip / apk containers.
  /// Needed to allow Android to read data from the apk.
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
