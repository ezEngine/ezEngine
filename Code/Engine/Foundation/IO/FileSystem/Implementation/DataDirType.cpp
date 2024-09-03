#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

ezResult ezDataDirectoryType::InitializeDataDirectory(ezStringView sDataDirPath)
{
  ezStringBuilder sPath = sDataDirPath;
  sPath.MakeCleanPath();

  EZ_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool ezDataDirectoryType::ExistsFile(ezStringView sFile, bool bOneSpecificDataDir)
{
  EZ_IGNORE_UNUSED(bOneSpecificDataDir);

  ezStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(sFile, sRedirectedAsset);

  ezStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return ezOSFile::ExistsFile(sPath);
}

void ezDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  ezFileSystem::FileEvent fe;
  fe.m_EventType = ezFileSystem::FileEventType::CloseFile;
  fe.m_sFileOrDirectory = GetFilePath();
  fe.m_pDataDir = m_pDataDirType;
  ezFileSystem::s_pData->m_Event.Broadcast(fe);

  m_pDataDirType->OnReaderWriterClose(this);
}


