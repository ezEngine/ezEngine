#include <FileservePlugin/FileservePluginPCH.h>

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
    ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, s_sRedirectionFile, true, nullptr).IgnoreResult();
  }

  FolderType::ReloadExternalConfigs();
}

ezDataDirectoryReader* ezDataDirectory::FileserveType::OpenFileToRead(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (ezPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  ezStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bSpecificallyThisDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  ezStringBuilder sFullPath;
  if (ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bSpecificallyThisDataDir, &sFullPath).Failed())
    return nullptr;

  // It's fine to use the base class here as it will resurface in CreateFolderReader which gives us control of the important part.
  return FolderType::OpenFileToRead(sFullPath, FileShareMode, bSpecificallyThisDataDir);
}

ezDataDirectoryWriter* ezDataDirectory::FileserveType::OpenFileToWrite(ezStringView sFile, ezFileShareMode::Enum FileShareMode)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (ezPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  return FolderType::OpenFileToWrite(sFile, FileShareMode);
}

ezResult ezDataDirectory::FileserveType::InternalInitializeDataDirectory(ezStringView sDirectory)
{
  ezStringBuilder sDataDir = sDirectory;
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

void ezDataDirectory::FileserveType::DeleteFile(ezStringView sFile)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, sFile);
  }

  FolderType::DeleteFile(sFile);
}

ezDataDirectory::FolderReader* ezDataDirectory::FileserveType::CreateFolderReader() const
{
  return EZ_DEFAULT_NEW(FileserveDataDirectoryReader, 0);
}

ezDataDirectory::FolderWriter* ezDataDirectory::FileserveType::CreateFolderWriter() const
{
  return EZ_DEFAULT_NEW(FileserveDataDirectoryWriter);
}

ezResult ezDataDirectory::FileserveType::GetFileStats(ezStringView sFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
{
  ezStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFileOrFolder, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return EZ_FAILURE;

  ezStringBuilder sFullPath;
  EZ_SUCCEED_OR_RETURN(ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  return ezOSFile::GetFileStats(sFullPath, out_Stats);
}

bool ezDataDirectory::FileserveType::ExistsFile(ezStringView sFile, bool bOneSpecificDataDir)
{
  ezStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return false;

  return ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, nullptr).Succeeded();
}

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(ezStringView sDataDirectory, ezStringView sGroup, ezStringView sRootName, ezDataDirUsage usage)
{
  if (!ezFileserveClient::s_bEnableFileserve || ezFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (sDataDirectory.IsEmpty())
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (!sDataDirectory.StartsWith(">"))
    return nullptr;

  if (ezFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  ezDataDirectory::FileserveType* pDataDir = EZ_DEFAULT_NEW(ezDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID = ezFileserveClient::GetSingleton()->MountDataDirectory(sDataDirectory, sRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(sDataDirectory) == EZ_SUCCESS)
    return pDataDir;

  EZ_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

ezDataDirectory::FileserveDataDirectoryReader::FileserveDataDirectoryReader(ezInt32 iDataDirUserData)
  : FolderReader(iDataDirUserData)
{
}

ezResult ezDataDirectory::FileserveDataDirectoryReader::InternalOpen(ezFileShareMode::Enum FileShareMode)
{
  return m_File.Open(GetFilePath().GetData(), ezFileOpenMode::Read, FileShareMode);
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
  if (file.Open(sAbsPath, ezFileOpenMode::Read).Failed())
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
