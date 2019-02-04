#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>

namespace ezDataDirectory
{
  class FolderReader;
  class FolderWriter;

  /// \brief A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at ezFileSystem to allow it to mount local directories.
  class EZ_FOUNDATION_DLL FolderType : public ezDataDirectoryType
  {
  public:
    ~FolderType();

    /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
    static ezDataDirectoryType* Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage);

    /// A 'redirection file' is an optional file located inside a data directory that lists which file access is redirected to which other file lookup.
    /// Each redirection is one line in the file (terminated by a \n).
    /// Each line consists of the 'key' string, a semicolon and a 'value' string. No unnecessary whitespace is allowed.
    /// When a file that matches 'key' is accessed through a mounted data directory, the file access will be replaced by 'value' (plus s_sRedirectionPrefix)
    /// 'key' may be anything (e.g. a GUID string), 'value' should be a valid relative path into the SAME data directory.
    /// The redirection file can be used to implement an asset lookup, where assets are identified by GUIDs and need to be mapped to the actual asset file.
    static ezString s_sRedirectionFile;

    /// If a redirection file is used AND the redirection lookup was successful, s_sRedirectionPrefix is prepended to the redirected file access.
    static ezString s_sRedirectionPrefix;

    /// \brief When s_sRedirectionFile and s_sRedirectionPrefix are used to enable file redirection, this will reload those config files.
    virtual void ReloadExternalConfigs() override;

    virtual const ezString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    // The implementations of the abstract functions.

    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir) override;

    virtual bool ResolveAssetRedirection(const char* szPathOrAssetGuid, ezStringBuilder& out_sRedirection) override;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(const char* szFile) override;
    virtual bool ExistsFile(const char* szFile, bool bOneSpecificDataDir) override;
    virtual ezResult GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const;
    virtual FolderWriter* CreateFolderWriter() const;

    /// \brief Called by 'ezDataDirectoryType_Folder::Factory'
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;

    /// \brief Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) override;

    void LoadRedirectionFile();

    mutable ezMutex m_ReaderWriterMutex; ///< Locks m_Readers / m_Writers as well as the m_bIsInUse flag of each reader / writer.
    ezHybridArray<ezDataDirectory::FolderReader*, 4> m_Readers;
    ezHybridArray<ezDataDirectory::FolderWriter*, 4> m_Writers;

    mutable ezMutex m_RedirectionMutex;
    ezMap<ezString, ezString> m_FileRedirection;
    ezString128 m_sRedirectedDataDirPath;
  };


  /// \brief Handles reading from ordinary files.
  class EZ_FOUNDATION_DLL FolderReader : public ezDataDirectoryReader
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader() { m_bIsInUse = false; }
    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;
    virtual ezUInt64 GetFileSize() const override;

  protected:
    virtual ezResult InternalOpen() override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    ezOSFile m_File;
  };

  /// \brief Handles writing to ordinary files.
  class EZ_FOUNDATION_DLL FolderWriter : public ezDataDirectoryWriter
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(FolderWriter);

  public:
    FolderWriter() { m_bIsInUse = false; }
    virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) override;
    virtual ezUInt64 GetFileSize() const override;

  protected:
    virtual ezResult InternalOpen() override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    ezOSFile m_File;
  };
}

