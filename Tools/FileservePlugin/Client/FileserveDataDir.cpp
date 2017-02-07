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
    ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirectionFile, dummy1, dummy2);
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
  ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, sRelPath, sAbsPath);

  return FolderType::OpenFileToRead(sRelPath);
}

ezDataDirectoryWriter* ezDataDirectory::FileserveType::OpenFileToWrite(const char* szFile)
{
  if (ezPathUtils::IsRootedPath(szFile))
  {
    ezLog::Error("rooted path");
  }

  if (ezPathUtils::IsAbsolutePath(szFile))
  {
    ezLog::Error("abs path");
  }

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

  ezStringBuilder sCacheFolder, sCacheMetaFolder;
  ezFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder, sCacheMetaFolder);
  m_sFileserveCacheFolder = sCacheFolder;
  m_sFileserveCacheMetaFolder = sCacheMetaFolder;

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

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (ezStringUtils::IsNullOrEmpty(szDataDirectory))
    return nullptr;

  //if (Usage == ezFileSystem::AllowWrites)
    //return nullptr;

  if (ezFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  ezDataDirectory::FileserveType* pDataDir = EZ_DEFAULT_NEW(ezDataDirectory::FileserveType);

  if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
  {
    const ezUInt16 uiDataDirID = ezFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory, szRootName);

    if (uiDataDirID < 0xffff)
    {
      pDataDir->m_uiDataDirID = uiDataDirID;
      return pDataDir;
    }
  }

  EZ_DEFAULT_DELETE(pDataDir);
  return nullptr;
}
