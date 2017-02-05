#include <PCH.h>
#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/Algorithm/Hashing.h>

bool ezDataDirectory::FileserveType::s_bEnableFileserve = true;

void GetFileserveCachePath(const char* szFile, ezStringBuilder& sNewPath, const char* szCacheFolder)
{
  sNewPath = szFile;
  sNewPath.RemoveFileExtension();

  const ezUInt32 uiFilenameHash = ezHashing::MurmurHash(sNewPath.GetData());
  sNewPath = szCacheFolder;
  sNewPath.AppendFormat("/{0}.{1}", ezArgU(uiFilenameHash, 8, true, 16), ezPathUtils::GetFileExtension(szFile));
}

ezDataDirectoryReader* ezDataDirectory::FileserveType::OpenFileToRead(const char* szFile)
{
  ezLog::Info("Reading '{0}'", szFile);

  ezStringBuilder sNewPath;
  GetFileserveCachePath(szFile, sNewPath, m_sFileserveCacheFolder);

  if (ezOSFile::ExistsFile(sNewPath))
  {
    ezLog::Success("File is already cached: '{0}'", szFile);
    return FolderType::OpenFileToRead(szFile);
  }
  else
  {
    ezFileserveClient::GetSingleton()->DownloadFile(szFile);
  }

  return FolderType::OpenFileToRead(szFile);
}

ezDataDirectoryWriter* ezDataDirectory::FileserveType::OpenFileToWrite(const char* szFile)
{
  ezLog::Info("Writing '{0}'", szFile);

  return FolderType::OpenFileToWrite(szFile);
}

ezResult ezDataDirectory::FileserveType::InternalInitializeDataDirectory(const char* szDirectory)
{
  m_sFileserveCacheFolder = ezOSFile::GetUserDataFolder("FileserveCache");

  if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
  {
    ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
    return EZ_FAILURE;
  }

  return FolderType::InternalInitializeDataDirectory(szDirectory);
}

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory)
{
  if (!s_bEnableFileserve)
    return nullptr;

  ezFileserveClient::GetSingleton()->EnsureConnected();
  ezFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory);

  {
    ezDataDirectory::FileserveType* pDataDir = EZ_DEFAULT_NEW(ezDataDirectory::FileserveType);

    if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
      return pDataDir;

    EZ_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }
}
