#include <Foundation/PCH.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

ezResult ezDataDirectoryType::InitializeDataDirectory(const char* szDataDirPath)
{
  ezStringBuilder sPath = szDataDirPath;
  sPath.MakeCleanPath();
  m_sDataDirectoryPath = sPath.GetData();

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool ezDataDirectoryType::ExistsFile(const char* szFile)
{
  ezStringBuilder sPath = m_sDataDirectoryPath.GetData();
  sPath.AppendPath(szFile);
  return ezOSFile::Exists(sPath.GetData());
}

void ezDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  ezFileSystem::FileEvent fe;
  fe.m_EventType = ezFileSystem::FileEventType::CloseFile;
  fe.m_szFileOrDirectory = GetFilePath ().GetData();
  fe.m_pDataDir = m_pDataDirectory;
  ezFileSystem::s_Data->m_Event.Broadcast(fe);


  m_pDataDirectory->OnReaderWriterClose(this);
}

