#include <Foundation/PCH.h>
#include <Foundation/IO/FileSystem/DataDirType_Folder.h>

ezResult ezDataDirectory_Folder_Reader::InternalOpen()
{
  ezStringBuilder sPath = GetDataDirectory()->GetDataDirectoryPath().GetData();
  sPath.AppendPath(GetFilePath().GetData());

  return m_File.Open(sPath.GetData(), ezFileMode::Read);
}

void ezDataDirectory_Folder_Reader::InternalClose()
{
  m_File.Close();
}

ezUInt64 ezDataDirectory_Folder_Reader::Read(void* pBuffer, ezUInt64 uiBytes)
{
  return m_File.Read(pBuffer, uiBytes);
}


ezResult ezDataDirectory_Folder_Writer::InternalOpen()
{
  ezStringBuilder sPath = GetDataDirectory()->GetDataDirectoryPath().GetData();
  sPath.AppendPath(GetFilePath().GetData());

  return m_File.Open(sPath.GetData(), ezFileMode::Write);
}

void ezDataDirectory_Folder_Writer::InternalClose()
{
  m_File.Close();
}

ezResult ezDataDirectory_Folder_Writer::Write(const void* pBuffer, ezUInt64 uiBytes)
{
  return m_File.Write(pBuffer, uiBytes);
}

ezDataDirectoryType* ezDataDirectoryType_Folder::Factory(const char* szDataDirectory)
{
  ezDataDirectoryType_Folder* pDataDir = EZ_DEFAULT_NEW(ezDataDirectoryType_Folder);

  if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
    return pDataDir;

  EZ_DEFAULT_DELETE(pDataDir);
  return NULL;
}

void ezDataDirectoryType_Folder::RemoveDataDirectory()
{
  for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
  {
    EZ_ASSERT(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
  }

  for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
  {
    EZ_ASSERT(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
  }

  ezDataDirectoryType_Folder* pThis = this;
  EZ_DEFAULT_DELETE(pThis);
}

void ezDataDirectoryType_Folder::DeleteFile(const char* szFile)
{
  ezStringBuilder sPath = GetDataDirectoryPath().GetData();
  sPath.AppendPath(szFile);

  ezOSFile::DeleteFile(sPath.GetData());
}

ezDataDirectoryType_Folder::~ezDataDirectoryType_Folder()
{
  for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    EZ_DEFAULT_DELETE(m_Readers[i]);

  for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
    EZ_DEFAULT_DELETE(m_Writers[i]);
}

ezResult ezDataDirectoryType_Folder::InternalInitializeDataDirectory(const char* szDirectory)
{
  // allow to set the 'empty' directory to handle all absolute paths
  if (ezStringUtils::IsNullOrEmpty(szDirectory))
    return EZ_SUCCESS;

  ezFileStats stats;
  if (ezOSFile::GetFileStats(szDirectory, stats) == EZ_FAILURE)
    return EZ_FAILURE;

  // If this is not a simple directory, this DataDirectoryType cannot mount it.
  if (!stats.m_bIsDirectory)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezDataDirectoryType_Folder::OnReaderWriterClose(ezDataDirectory_ReaderWriter_Base* pClosed)
{
  if (pClosed->IsReader())
  {
    ezDataDirectory_Folder_Reader* pReader = (ezDataDirectory_Folder_Reader*) pClosed;
    pReader->m_bIsInUse = false;
  }
  else
  {
    ezDataDirectory_Folder_Writer* pWriter = (ezDataDirectory_Folder_Writer*) pClosed;
    pWriter->m_bIsInUse = false;
  }
}

ezDataDirectory_Reader* ezDataDirectoryType_Folder::OpenFileToRead(const char* szFile)
{
  ezDataDirectory_Folder_Reader* pReader = NULL;

  for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
  {
    if (!m_Readers[i]->m_bIsInUse)
      pReader = m_Readers[i];
  }

  if (pReader == NULL)
  {
    m_Readers.Push(EZ_DEFAULT_NEW(ezDataDirectory_Folder_Reader));
    pReader = m_Readers.Peek();
  }

  // if opening the file fails, the reader state is never set to 'used', so nothing else needs to be done
  if (pReader->Open(szFile, this) == EZ_FAILURE)
    return NULL;

  // if it succeeds, we return the reader
  pReader->m_bIsInUse = true;
  return pReader;
}

ezDataDirectory_Writer* ezDataDirectoryType_Folder::OpenFileToWrite(const char* szFile)
{
  ezDataDirectory_Folder_Writer* pWriter = NULL;

  for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
  {
    if (!m_Writers[i]->m_bIsInUse)
      pWriter = m_Writers[i];
  }

  if (pWriter == NULL)
  {
    m_Writers.Push(EZ_DEFAULT_NEW(ezDataDirectory_Folder_Writer));
    pWriter = m_Writers.Peek();
  }

  // if opening the file fails, the writer state is never set to 'used', so nothing else needs to be done
  if (pWriter->Open(szFile, this) == EZ_FAILURE)
    return NULL;

  // if it succeeds, we return the reader
  pWriter->m_bIsInUse = true;
  return pWriter;
}

