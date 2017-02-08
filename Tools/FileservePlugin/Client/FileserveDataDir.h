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
  /// \brief A data directory type to handle access to files that are served from a network host.
  class EZ_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
    static ezDataDirectoryType* Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage);

    /// \brief [internal] Fileserve caches all files in a writable user folder for actual disk access.
    virtual const ezString128& GetRedirectedDataDirectoryPath() const override { return m_sFileserveCacheFolder; }

    /// \brief [internal] Makes sure the redirection config files are up to date and then reloads them.
    virtual void ReloadExternalConfigs() override;

  protected:
    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile) override;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override;
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;

    /// \todo Implement these
    //virtual void RemoveDataDirectory() override {}
    //virtual void DeleteFile(const char* szFile) override {}

    ezUInt16 m_uiDataDirID = 0xffff;
    ezString128 m_sFileserveCacheFolder;
    ezString128 m_sFileserveCacheMetaFolder;
  };
}



