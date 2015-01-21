#include <Foundation/PCH.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

namespace ezDataDirectory
{

  ezResult FolderReader::InternalOpen()
  {
    ezStringBuilder sPath = GetDataDirectory()->GetDataDirectoryPath();
    sPath.AppendPath(GetFilePath().GetData());

    return m_File.Open(sPath.GetData(), ezFileMode::Read);
  }

  void FolderReader::InternalClose()
  {
    m_File.Close();
  }

  ezUInt64 FolderReader::Read(void* pBuffer, ezUInt64 uiBytes)
  {
    return m_File.Read(pBuffer, uiBytes);
  }

  ezUInt64 FolderReader::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  ezResult FolderWriter::InternalOpen()
  {
    ezStringBuilder sPath = GetDataDirectory()->GetDataDirectoryPath();
    sPath.AppendPath(GetFilePath().GetData());

    return m_File.Open(sPath.GetData(), ezFileMode::Write);
  }

  void FolderWriter::InternalClose()
  {
    m_File.Close();
  }

  ezResult FolderWriter::Write(const void* pBuffer, ezUInt64 uiBytes)
  {
    return m_File.Write(pBuffer, uiBytes);
  }

  ezUInt64 FolderWriter::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  ezDataDirectoryType* FolderType::Factory(const char* szDataDirectory)
  {
    FolderType* pDataDir = EZ_DEFAULT_NEW(FolderType);

    if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
      return pDataDir;

    EZ_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }

  void FolderType::RemoveDataDirectory()
  {
    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    {
      EZ_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
    }

    for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
    {
      EZ_ASSERT_DEV(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
    }

    FolderType* pThis = this;
    EZ_DEFAULT_DELETE(pThis);
  }

  void FolderType::DeleteFile(const char* szFile)
  {
    ezStringBuilder sPath = GetDataDirectoryPath();
    sPath.AppendPath(szFile);

    ezOSFile::DeleteFile(sPath.GetData());
  }

  FolderType::~FolderType()
  {
    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      EZ_DEFAULT_DELETE(m_Readers[i]);

    for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      EZ_DEFAULT_DELETE(m_Writers[i]);
  }

  ezResult FolderType::InternalInitializeDataDirectory(const char* szDirectory)
  {
    // allow to set the 'empty' directory to handle all absolute paths
    if (ezStringUtils::IsNullOrEmpty(szDirectory))
      return EZ_SUCCESS;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

    ezFileStats stats;
    if (ezOSFile::GetFileStats(szDirectory, stats) == EZ_FAILURE)
      return EZ_FAILURE;

    // If this is not a simple directory, this DataDirectoryType cannot mount it.
    if (!stats.m_bIsDirectory)
      return EZ_FAILURE;

#endif

    return EZ_SUCCESS;
  }

  void FolderType::OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed)
  {
    if (pClosed->IsReader())
    {
      FolderReader* pReader = (FolderReader*) pClosed;
      pReader->m_bIsInUse = false;
    }
    else
    {
      FolderWriter* pWriter = (FolderWriter*) pClosed;
      pWriter->m_bIsInUse = false;
    }
  }

  ezDataDirectoryReader* FolderType::OpenFileToRead(const char* szFile)
  {
    FolderReader* pReader = nullptr;

    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    {
      if (!m_Readers[i]->m_bIsInUse)
        pReader = m_Readers[i];
    }

    if (pReader == nullptr)
    {
      m_Readers.PushBack(EZ_DEFAULT_NEW(FolderReader));
      pReader = m_Readers.PeekBack();
    }

    // if opening the file fails, the reader state is never set to 'used', so nothing else needs to be done
    if (pReader->Open(szFile, this) == EZ_FAILURE)
      return nullptr;

    // if it succeeds, we return the reader
    pReader->m_bIsInUse = true;
    return pReader;
  }

  ezDataDirectoryWriter* FolderType::OpenFileToWrite(const char* szFile)
  {
    FolderWriter* pWriter = nullptr;

    for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
    {
      if (!m_Writers[i]->m_bIsInUse)
        pWriter = m_Writers[i];
    }

    if (pWriter == nullptr)
    {
      m_Writers.PushBack(EZ_DEFAULT_NEW(FolderWriter));
      pWriter = m_Writers.PeekBack();
    }

    // if opening the file fails, the writer state is never set to 'used', so nothing else needs to be done
    if (pWriter->Open(szFile, this) == EZ_FAILURE)
      return nullptr;

    // if it succeeds, we return the reader
    pWriter->m_bIsInUse = true;
    return pWriter;
  }

}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeFolder);

