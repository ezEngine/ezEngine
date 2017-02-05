#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

namespace ezDataDirectory
{
  class FileserveReader;
  class FileserveWriter;

  /// \brief A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at ezFileSystem to allow it to mount local directories.
  class EZ_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    //~FileserveType();

    /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
    static ezDataDirectoryType* Factory(const char* szDataDirectory);

    static bool s_bEnableFileserve;

  protected:
    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile) override;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override;
    //virtual void RemoveDataDirectory() override {}
    //virtual void DeleteFile(const char* szFile) override {}
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;

    ezString m_sFileserveCacheFolder;
  };
}



