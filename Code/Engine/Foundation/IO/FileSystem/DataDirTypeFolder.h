#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>

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

  protected:
    // The implementations of the abstract functions.

    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile) EZ_OVERRIDE;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) EZ_OVERRIDE;
    virtual void RemoveDataDirectory() EZ_OVERRIDE;
    virtual void DeleteFile(const char* szFile) EZ_OVERRIDE;

  private:
    /// \brief Called by 'ezDataDirectoryType_Folder::Factory'
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) EZ_OVERRIDE;

    /// \brief Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) EZ_OVERRIDE;

    ezHybridArray<ezDataDirectory::FolderReader*, 4> m_Readers;
    ezHybridArray<ezDataDirectory::FolderWriter*, 4> m_Writers;
  };


  /// \brief Handles reading from ordinary files.
  class EZ_FOUNDATION_DLL FolderReader : public ezDataDirectoryReader
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader() { m_bIsInUse = false; } 
    virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) EZ_OVERRIDE;

  private:
    virtual ezResult InternalOpen() EZ_OVERRIDE;
    virtual void InternalClose() EZ_OVERRIDE;

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
    virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) EZ_OVERRIDE;

  private:
    virtual ezResult InternalOpen() EZ_OVERRIDE;
    virtual void InternalClose() EZ_OVERRIDE;

    friend class FolderType;

    bool m_bIsInUse;
    ezOSFile m_File;
  };

}



