#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <FileservePlugin/FileserveClient.h>

namespace ezDataDirectory
{
  class FileserveReader;
  class FileserveWriter;

  /// \brief A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at ezFileSystem to allow it to mount local directories.
  class EZ_FILESERVEPLUGIN_DLL FileserveType : public ezDataDirectoryType
  {
  public:
    //~FileserveType();

    /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
    static ezDataDirectoryType* Factory(const char* szDataDirectory);

    static bool s_bEnableFileserve;
    virtual void ReloadExternalConfigs() override {}

  protected:
    // The implementations of the abstract functions.

    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile) override { return nullptr; }
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override { return nullptr; }
    virtual void RemoveDataDirectory() override {}
    virtual void DeleteFile(const char* szFile) override {}

  private:
    /// \brief Called by 'ezDataDirectoryType_Folder::Factory'
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override { return EZ_FAILURE; }

    /// \brief Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) override { }
  };

  ///// \brief Handles reading from ordinary files.
  //class EZ_FILESERVEPLUGIN_DLL FileserveReader : public ezDataDirectoryReader
  //{
  //  EZ_DISALLOW_COPY_AND_ASSIGN(FileserveReader);

  //public:
  //  FileserveReader() { m_bIsInUse = false; }
  //  virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;
  //  virtual ezUInt64 GetFileSize() const override;

  //private:
  //  virtual ezResult InternalOpen() override;
  //  virtual void InternalClose() override;

  //  friend class FolderType;

  //  bool m_bIsInUse;
  //  ezOSFile m_File;
  //};

  ///// \brief Handles writing to ordinary files.
  //class EZ_FILESERVEPLUGIN_DLL FileserveWriter : public ezDataDirectoryWriter
  //{
  //  EZ_DISALLOW_COPY_AND_ASSIGN(FileserveWriter);

  //public:
  //  FileserveWriter() { m_bIsInUse = false; }
  //  virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) override;
  //  virtual ezUInt64 GetFileSize() const override;

  //private:
  //  virtual ezResult InternalOpen() override;
  //  virtual void InternalClose() override;

  //  friend class FolderType;

  //  bool m_bIsInUse;
  //  ezOSFile m_File;
  //};

}



