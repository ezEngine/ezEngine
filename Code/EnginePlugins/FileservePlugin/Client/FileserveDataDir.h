#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

namespace ezDataDirectory
{
  class FileserveDataDirectoryWriter : public FolderWriter
  {
  protected:
    virtual void InternalClose() override;

  };

  /// \brief A data directory type to handle access to files that are served from a network host.
  class EZ_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
    static ezDataDirectoryType* Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage);

    /// \brief [internal] Makes sure the redirection config files are up to date and then reloads them.
    virtual void ReloadExternalConfigs() override;

    /// \brief [internal] Called by FileserveDataDirectoryWriter when it is finished to upload the written file to the server
    void FinishedWriting(FolderWriter* pWriter);

  protected:
    virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir) override;
    virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) override;
    virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(const char* szFile) override;
    virtual bool ExistsFile(const char* szFile, bool bOneSpecificDataDir) override;
    /// \brief Limitation: Fileserve does not handle folders, only files. If someone stats a folder, this will fail.
    virtual ezResult GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) override;
    virtual FolderWriter* CreateFolderWriter() const override;

    ezUInt16 m_uiDataDirID = 0xffff;
    ezString128 m_sFileserveCacheMetaFolder;
  };
}



