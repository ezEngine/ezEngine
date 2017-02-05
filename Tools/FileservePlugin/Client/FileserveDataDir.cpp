#include <PCH.h>
#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/Algorithm/Hashing.h>

bool ezDataDirectory::FileserveType::s_bEnableFileserve = true;



void ezDataDirectory::FileserveType::ReloadExternalConfigs()
{
  EZ_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (!s_sRedirectionFile.IsEmpty())
  {
    ezStringBuilder sRedirectionFile(GetDataDirectoryPath(), "/", s_sRedirectionFile);
    sRedirectionFile.MakeCleanPath();

    ezStringBuilder dummy1, dummy2;
    ezFileserveClient::GetSingleton()->DownloadFile(sRedirectionFile, dummy1, dummy2);
  }

  FolderType::ReloadExternalConfigs();
}

ezDataDirectoryReader* ezDataDirectory::FileserveType::OpenFileToRead(const char* szFile)
{
  ezStringBuilder sRedirected;
  if (UseFileRedirection(szFile, sRedirected))
  {
    /// \todo Should we prepend the data dir path, to make it absolutely clear in which data dir the file was found?
    int i = 0;
    (void)i;
  }

  ezStringBuilder sRelPath, sAbsPath;
  ezFileserveClient::GetSingleton()->DownloadFile(sRedirected, sRelPath, sAbsPath);

  return FolderType::OpenFileToRead(sRelPath);
}

ezDataDirectoryWriter* ezDataDirectory::FileserveType::OpenFileToWrite(const char* szFile)
{
  ezLog::Info("Writing '{0}'", szFile);

  //if (ezStringUtils::StartsWith(szFile, GetDataDirectoryPath()))
  //    szFile += GetDataDirectoryPath().GetElementCount();

  //while (szFile[0] == '/' || szFile[0] == '\\')
  //  szFile += 1;

  //ezStringBuilder sNewPath;
  //ezFileserveClient::GetSingleton()->GetFileserveCachePath(szFile, sNewPath);

  return FolderType::OpenFileToWrite(szFile);
}

ezResult ezDataDirectory::FileserveType::InternalInitializeDataDirectory(const char* szDirectory)
{
  ezStringBuilder sDataDir = szDirectory;
  sDataDir.MakeCleanPath();
  if (!sDataDir.IsEmpty() && !sDataDir.EndsWith("/"))
    sDataDir.Append("/");

  ezStringBuilder sCacheFolder;
  ezFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder);
  m_sFileserveCacheFolder = sCacheFolder;

  //if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
  //{
  //  ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder.GetData());
  //  return EZ_FAILURE;
  //}

  return FolderType::InternalInitializeDataDirectory(sDataDir);
}

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage)
{
  if (!s_bEnableFileserve)
    return nullptr;

  //if (Usage == ezFileSystem::AllowWrites)
    //return nullptr;

  if (ezFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  ezFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory, szRootName);

  {
    ezDataDirectory::FileserveType* pDataDir = EZ_DEFAULT_NEW(ezDataDirectory::FileserveType);

    if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
      return pDataDir;

    EZ_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }
}
