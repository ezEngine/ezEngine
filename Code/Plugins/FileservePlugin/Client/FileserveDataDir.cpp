#include <FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/Logging/Log.h>

void ezDataDirectory::FileserveType::ReloadExternalConfigs()
{
  EZ_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (!s_sRedirectionFile.IsEmpty())
  {
    ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, s_sRedirectionFile, true);
  }

  FolderType::ReloadExternalConfigs();
}

ezDataDirectoryReader* ezDataDirectory::FileserveType::OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (ezPathUtils::IsAbsolutePath(szFile))
    return nullptr;

  ezStringBuilder sRedirected;
  ResolveAssetRedirection(szFile, sRedirected);

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  if (ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bSpecificallyThisDataDir).Failed())
    return nullptr;

  return FolderType::OpenFileToRead(sRedirected, bSpecificallyThisDataDir);
}

ezDataDirectoryWriter* ezDataDirectory::FileserveType::OpenFileToWrite(const char* szFile)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (ezPathUtils::IsAbsolutePath(szFile))
    return nullptr;

  return FolderType::OpenFileToWrite(szFile);
}

ezResult ezDataDirectory::FileserveType::InternalInitializeDataDirectory(const char* szDirectory)
{
  ezStringBuilder sDataDir = szDirectory;
  sDataDir.MakeCleanPath();

  ezStringBuilder sCacheFolder, sCacheMetaFolder;
  ezFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder, sCacheMetaFolder);
  m_sRedirectedDataDirPath = sCacheFolder;
  m_sFileserveCacheMetaFolder = sCacheMetaFolder;

  ReloadExternalConfigs();
  return EZ_SUCCESS;
}

void ezDataDirectory::FileserveType::RemoveDataDirectory()
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UnmountDataDirectory(m_uiDataDirID);
  }

  FolderType::RemoveDataDirectory();
}

void ezDataDirectory::FileserveType::DeleteFile(const char* szFile)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, szFile);
  }

  FolderType::DeleteFile(szFile);
}

ezDataDirectory::FolderWriter* ezDataDirectory::FileserveType::CreateFolderWriter() const
{
  return EZ_DEFAULT_NEW(FileserveDataDirectoryWriter);
}


ezResult ezDataDirectory::FileserveType::GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
{
  EZ_SUCCEED_OR_RETURN(ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, szFileOrFolder, bOneSpecificDataDir));
  return FolderType::GetFileStats(szFileOrFolder, bOneSpecificDataDir, out_Stats);
}

bool ezDataDirectory::FileserveType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
{
  /// \todo Not sure whether this needs to be done here
  // ezStringBuilder sRedirectedAsset;
  // ResolveAssetRedirection(szFile, sRedirectedAsset);

  return ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, szFile, bOneSpecificDataDir).Succeeded();
}

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory, const char* szGroup, const char* szRootName,
                                                             ezFileSystem::DataDirUsage Usage)
{
  if (!ezFileserveClient::s_bEnableFileserve || ezFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (ezStringUtils::IsNullOrEmpty(szDataDirectory))
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (szDataDirectory[0] != '>')
    return nullptr;

  if (ezFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  ezDataDirectory::FileserveType* pDataDir = EZ_DEFAULT_NEW(ezDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID = ezFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory, szRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
    return pDataDir;

  EZ_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

void ezDataDirectory::FileserveDataDirectoryWriter::InternalClose()
{
  FolderWriter::InternalClose();

  static_cast<FileserveType*>(GetDataDirectory())->FinishedWriting(this);
}

void ezDataDirectory::FileserveType::FinishedWriting(FolderWriter* pWriter)
{
  if (ezFileserveClient::GetSingleton() == nullptr)
    return;

  ezStringBuilder sAbsPath = pWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
  sAbsPath.AppendPath(pWriter->GetFilePath());

  ezOSFile file;
  if (file.Open(sAbsPath, ezFileMode::Read).Failed())
  {
    ezLog::Error("Could not read file for upload: '{0}'", sAbsPath);
    return;
  }

  ezDynamicArray<ezUInt8> content;
  file.ReadAll(content);
  file.Close();

  ezFileserveClient::GetSingleton()->UploadFile(m_uiDataDirID, pWriter->GetFilePath(), content);
}



EZ_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveDataDir);
