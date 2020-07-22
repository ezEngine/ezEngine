#include <FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

ezResult ezDataDirectoryType::InitializeDataDirectory(const char* szDataDirPath)
{
  ezStringBuilder sPath = szDataDirPath;
  sPath.MakeCleanPath();

  EZ_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool ezDataDirectoryType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
{
  ezStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(szFile, sRedirectedAsset);

  ezStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return ezOSFile::ExistsFile(sPath);
}

void ezDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  ezFileSystem::FileEvent fe;
  fe.m_EventType = ezFileSystem::FileEventType::CloseFile;
  fe.m_szFileOrDirectory = GetFilePath().GetData();
  fe.m_pDataDir = m_pDataDirectory;
  ezFileSystem::s_Data->m_Event.Broadcast(fe);

  m_pDataDirectory->OnReaderWriterClose(this);
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirType);
