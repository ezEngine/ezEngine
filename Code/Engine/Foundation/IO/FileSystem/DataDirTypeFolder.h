#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>

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
    static ezDataDirectoryType* Factory(const char* szDataDirectory);

    /// A 'redirection file' is an optional file located inside a data directory that lists which file access is redirected to which other file lookup.
    /// Each redirection is one line in the file (terminated by a \n).
    /// Each line consists of the 'key' string, a semicolon and a 'value' string. No unnecessary whitespace is allowed.
    /// When a file that matches 'key' is accessed through a mounted data directory, the file access will be replaced by 'value' (plus s_sRedirectionPrefix)
    /// 'key' may be anything (e.g. a GUID string), 'value' should be a valid relative path into the SAME data directory.
    /// The redirection file can be used to implement an asset lookup, where assets are identified by GUIDs and need to be mapped to the actual asset file.
    static ezString s_sRedirectionFile;

    /// If a redirection file is used AND the redirection lookup was successful, s_sRedirectionPrefix is prepended to the redirected file access.
    static ezString s_sRedirectionPrefix;

    virtual void ReloadExternalConfigs() override;

  protected:
    // The implementations of the abstract functions.

    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile) override;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(const char* szFile) override;

  private:
    /// \brief Called by 'ezDataDirectoryType_Folder::Factory'
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;

    /// \brief Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) override;

    void LoadRedirectionFile();

    ezHybridArray<ezDataDirectory::FolderReader*, 4> m_Readers;
    ezHybridArray<ezDataDirectory::FolderWriter*, 4> m_Writers;

    ezMap<ezString, ezString> m_FileRedirection;
  };


  /// \brief Handles reading from ordinary files.
  class EZ_FOUNDATION_DLL FolderReader : public ezDataDirectoryReader
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader() { m_bIsInUse = false; } 
    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;
    virtual ezUInt64 GetFileSize() const override;

  private:
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

  private:
    virtual ezResult InternalOpen() override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    ezOSFile m_File;
  };

}



