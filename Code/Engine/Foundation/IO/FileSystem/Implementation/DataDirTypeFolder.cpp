#include <FoundationPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>

namespace ezDataDirectory
{
  ezString FolderType::s_sRedirectionFile;
  ezString FolderType::s_sRedirectionPrefix;

  ezResult FolderReader::InternalOpen()
  {
    ezStringBuilder sPath = ((ezDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath().GetData());

    return m_File.Open(sPath.GetData(), ezFileMode::Read);
  }

  void FolderReader::InternalClose() { m_File.Close(); }

  ezUInt64 FolderReader::Read(void* pBuffer, ezUInt64 uiBytes) { return m_File.Read(pBuffer, uiBytes); }

  ezUInt64 FolderReader::GetFileSize() const { return m_File.GetFileSize(); }

  ezResult FolderWriter::InternalOpen()
  {
    ezStringBuilder sPath = ((ezDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath().GetData());

    return m_File.Open(sPath.GetData(), ezFileMode::Write);
  }

  void FolderWriter::InternalClose() { m_File.Close(); }

  ezResult FolderWriter::Write(const void* pBuffer, ezUInt64 uiBytes) { return m_File.Write(pBuffer, uiBytes); }

  ezUInt64 FolderWriter::GetFileSize() const { return m_File.GetFileSize(); }

  ezDataDirectoryType* FolderType::Factory(
    const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage)
  {
    FolderType* pDataDir = EZ_DEFAULT_NEW(FolderType);

    if (pDataDir->InitializeDataDirectory(szDataDirectory) == EZ_SUCCESS)
      return pDataDir;

    EZ_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }

  void FolderType::RemoveDataDirectory()
  {
    {
      EZ_LOCK(m_ReaderWriterMutex);
      for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        EZ_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }

      for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        EZ_ASSERT_DEV(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }
    }
    FolderType* pThis = this;
    EZ_DEFAULT_DELETE(pThis);
  }

  void FolderType::DeleteFile(const char* szFile)
  {
    ezStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(szFile);

    ezOSFile::DeleteFile(sPath.GetData());
  }

  FolderType::~FolderType()
  {
    EZ_LOCK(m_ReaderWriterMutex);
    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      EZ_DEFAULT_DELETE(m_Readers[i]);

    for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      EZ_DEFAULT_DELETE(m_Writers[i]);
  }

  void FolderType::ReloadExternalConfigs() { LoadRedirectionFile(); }

  void FolderType::LoadRedirectionFile()
  {
    EZ_LOCK(m_RedirectionMutex);
    m_FileRedirection.Clear();

    if (!s_sRedirectionFile.IsEmpty())
    {
      ezStringBuilder sRedirectionFile(GetRedirectedDataDirectoryPath(), "/", s_sRedirectionFile);
      sRedirectionFile.MakeCleanPath();

      EZ_LOG_BLOCK("LoadRedirectionFile", sRedirectionFile.GetData());

      ezOSFile file;
      if (file.Open(sRedirectionFile, ezFileMode::Read).Succeeded())
      {
        ezHybridArray<char, 1024 * 10> content;
        char uiTemp[4096];

        ezUInt64 uiRead = 0;

        do
        {
          uiRead = file.Read(uiTemp, EZ_ARRAY_SIZE(uiTemp));
          content.PushBackRange(ezArrayPtr<char>(uiTemp, (ezUInt32)uiRead));
        } while (uiRead == EZ_ARRAY_SIZE(uiTemp));

        content.PushBack(0); // make sure the string is terminated

        const char* szLineStart = content.GetData();
        const char* szSeparator = nullptr;
        const char* szLineEnd = nullptr;

        ezStringBuilder sFileToRedirect, sRedirection;

        while (true)
        {
          szSeparator = ezStringUtils::FindSubString(szLineStart, ";");
          szLineEnd = ezStringUtils::FindSubString(szSeparator, "\n");

          if (szLineStart == nullptr || szSeparator == nullptr || szLineEnd == nullptr)
            break;

          sFileToRedirect.SetSubString_FromTo(szLineStart, szSeparator);
          sRedirection.SetSubString_FromTo(szSeparator + 1, szLineEnd);

          m_FileRedirection[sFileToRedirect] = sRedirection;

          szLineStart = szLineEnd + 1;
        }

        // ezLog::Debug("Redirection file contains {0} entries", m_FileRedirection.GetCount());
      }
      // else
      // ezLog::Debug("No Redirection file found in: '{0}'", sRedirectionFile);
    }
  }


  bool FolderType::ExistsFile(const char* szFile, bool bOneSpecificDataDir)
  {
    ezStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(szFile, sRedirectedAsset);

    ezStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sRedirectedAsset);
    return ezOSFile::ExistsFile(sPath);
  }

  ezResult FolderType::GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
  {
    ezStringBuilder sPath = GetRedirectedDataDirectoryPath();

    if (ezPathUtils::IsAbsolutePath(szFileOrFolder))
    {
      if (!ezStringUtils::StartsWith_NoCase(szFileOrFolder, sPath))
        return EZ_FAILURE;

      sPath.Clear();
    }

    sPath.AppendPath(szFileOrFolder);
    return ezOSFile::GetFileStats(sPath, out_Stats);
  }

  ezResult FolderType::InternalInitializeDataDirectory(const char* szDirectory)
  {
    // allow to set the 'empty' directory to handle all absolute paths
    if (ezStringUtils::IsNullOrEmpty(szDirectory))
      return EZ_SUCCESS;

    ezStringBuilder sRedirected;
    if (ezFileSystem::ResolveSpecialDirectory(szDirectory, sRedirected).Succeeded())
    {
      m_sRedirectedDataDirPath = sRedirected;
    }
    else
    {
      m_sRedirectedDataDirPath = szDirectory;
    }

    if (!ezOSFile::ExistsDirectory(m_sRedirectedDataDirPath))
      return EZ_FAILURE;

    ReloadExternalConfigs();

    return EZ_SUCCESS;
  }

  void FolderType::OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed)
  {
    EZ_LOCK(m_ReaderWriterMutex);
    if (pClosed->IsReader())
    {
      FolderReader* pReader = (FolderReader*)pClosed;
      pReader->m_bIsInUse = false;
    }
    else
    {
      FolderWriter* pWriter = (FolderWriter*)pClosed;
      pWriter->m_bIsInUse = false;
    }
  }

  ezDataDirectory::FolderReader* FolderType::CreateFolderReader() const { return EZ_DEFAULT_NEW(FolderReader, 0); }

  ezDataDirectory::FolderWriter* FolderType::CreateFolderWriter() const { return EZ_DEFAULT_NEW(FolderWriter, 0); }

  ezDataDirectoryReader* FolderType::OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir)
  {
    ezStringBuilder sFileToOpen;
    ResolveAssetRedirection(szFile, sFileToOpen);

    // we know that these files cannot be opened, so don't even try
    if (ezConversionUtils::IsStringUuid(sFileToOpen))
      return nullptr;

    FolderReader* pReader = nullptr;
    {
      EZ_LOCK(m_ReaderWriterMutex);
      for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        if (!m_Readers[i]->m_bIsInUse)
          pReader = m_Readers[i];
      }

      if (pReader == nullptr)
      {
        m_Readers.PushBack(CreateFolderReader());
        pReader = m_Readers.PeekBack();
      }
      pReader->m_bIsInUse = true;
    }

    // if opening the file fails, the reader's m_bIsInUse needs to be reset.
    if (pReader->Open(sFileToOpen, this) == EZ_FAILURE)
    {
      EZ_LOCK(m_ReaderWriterMutex);
      pReader->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pReader;
  }


  bool FolderType::ResolveAssetRedirection(const char* szFile, ezStringBuilder& out_sRedirection)
  {
    EZ_LOCK(m_RedirectionMutex);
    // Check if we know about a file redirection for this
    auto it = m_FileRedirection.Find(szFile);

    // if available, open the file that is mentioned in the redirection file instead
    if (it.IsValid())
    {

      if (it.Value().StartsWith("?"))
      {
        // ? is an option to tell the system to skip the redirection prefix and use the path as is
        out_sRedirection = &it.Value().GetData()[1];
      }
      else
      {
        out_sRedirection.Set(s_sRedirectionPrefix, it.Value());
      }
      return true;
    }
    else
    {
      out_sRedirection = szFile;
      return false;
    }
  }

  ezDataDirectoryWriter* FolderType::OpenFileToWrite(const char* szFile)
  {
    FolderWriter* pWriter = nullptr;

    {
      EZ_LOCK(m_ReaderWriterMutex);
      for (ezUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        if (!m_Writers[i]->m_bIsInUse)
          pWriter = m_Writers[i];
      }

      if (pWriter == nullptr)
      {
        m_Writers.PushBack(CreateFolderWriter());
        pWriter = m_Writers.PeekBack();
      }
      pWriter->m_bIsInUse = true;
    }
    // if opening the file fails, the writer's m_bIsInUse needs to be reset.
    if (pWriter->Open(szFile, this) == EZ_FAILURE)
    {
      EZ_LOCK(m_ReaderWriterMutex);
      pWriter->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pWriter;
  }
} // namespace ezDataDirectory



EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeFolder);
