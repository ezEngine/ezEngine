#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeWeb.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

#if EZ_ENABLED(EZ_PLATFORM_WEB)
#  include <emscripten/emscripten.h>
#  include <emscripten/fetch.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, DataDirectoryWeb)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectoryTypeWeb::Factory);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

#endif

// TODO Web: Generate inventory file to fulfill GetFileStats requests and know what to fetch and what not
// TODO Web: option to pre-fetch files

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
bool ezDataDirectoryTypeWeb::IgnoreClientCache = false;
ezString ezDataDirectoryTypeWeb::s_sRedirectionFile;
ezString ezDataDirectoryTypeWeb::s_sRedirectionPrefix;
#endif

void ezDataDirectoryTypeWeb::LoadRedirectionFile()
{
  EZ_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (s_sRedirectionFile.IsEmpty())
    return;

  ezStringBuilder sRedirectionFile(GetRedirectedDataDirectoryPath(), "/", s_sRedirectionFile);
  sRedirectionFile.MakeCleanPath();

  EZ_LOG_BLOCK("LoadRedirectionFile", sRedirectionFile);

  ezHybridArray<char, 1024 * 10> content;

#if EZ_ENABLED(EZ_PLATFORM_WEB)
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  ezStringUtils::Copy(attr.requestMethod, 32, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;

  emscripten_fetch_t* fetch = emscripten_fetch(&attr, sRedirectionFile.GetData());
  if (fetch->status == 200)
  {
    content.SetCountUninitialized(fetch->numBytes);
    ezMemoryUtils::RawByteCopy(content.GetData(), fetch->data, content.GetCount());

    ezLog::Success("Fetched web redirection file: '{}' - {}", fetch->url, ezArgFileSize(fetch->numBytes));
  }
  else
  {
    ezLog::Warning("Failed to fetch redirection file: '{}' - Code {}", fetch->url, fetch->status);
    return;
  }
  emscripten_fetch_close(fetch);
#endif

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
}

void ezDataDirectoryTypeWeb::ReloadExternalConfigs()
{
  LoadRedirectionFile();
}

ezDataDirectoryReader* ezDataDirectoryTypeWeb::OpenFileToRead(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  EZ_IGNORE_UNUSED(bSpecificallyThisDataDir);

  // web servers cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (ezPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  ezStringBuilder sRedirected;
  ResolveAssetRedirection(sFile, sRedirected);

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  ezDataDirectoryReaderWeb* pReader = nullptr;
  {
    EZ_LOCK(m_ReaderWriterMutex);
    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    {
      if (!m_Readers[i]->m_bIsInUse)
        pReader = m_Readers[i];
    }

    if (pReader == nullptr)
    {
      m_Readers.PushBack(EZ_DEFAULT_NEW(ezDataDirectoryReaderWeb));
      pReader = m_Readers.PeekBack();
    }
    pReader->m_bIsInUse = true;
  }

  // if opening the file fails, the reader's m_bIsInUse needs to be reset.
  if (pReader->Open(sRedirected, this, FileShareMode) == EZ_FAILURE)
  {
    EZ_LOCK(m_ReaderWriterMutex);
    pReader->m_bIsInUse = false;
    return nullptr;
  }

  // if it succeeds, we return the reader
  return pReader;
}

bool ezDataDirectoryTypeWeb::ResolveAssetRedirection(ezStringView sPathOrAssetGuid, ezStringBuilder& out_sRedirection)
{
  EZ_LOCK(m_RedirectionMutex);
  // Check if we know about a file redirection for this
  auto it = m_FileRedirection.Find(sPathOrAssetGuid);

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
    out_sRedirection = sPathOrAssetGuid;
    return false;
  }
}

ezResult ezDataDirectoryTypeWeb::InternalInitializeDataDirectory(ezStringView sDirectory)
{
  EZ_IGNORE_UNUSED(sDirectory);

  ReloadExternalConfigs();
  return EZ_SUCCESS;
}

ezResult ezDataDirectoryTypeWeb::GetFileStats(ezStringView sFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats)
{
  EZ_IGNORE_UNUSED(bOneSpecificDataDir);

  ezStringBuilder sRedirected;
  ResolveAssetRedirection(sFileOrFolder, sRedirected);

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return EZ_FAILURE;

  // TODO Web: ezDataDirectoryTypeWeb::GetFileStats

  // ezStringBuilder sFullPath;
  // EZ_SUCCEED_OR_RETURN(ezFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  // return ezOSFile::GetFileStats(sFullPath, out_Stats);
  EZ_IGNORE_UNUSED(out_Stats);
  return EZ_FAILURE;
}

void ezDataDirectoryTypeWeb::RemoveDataDirectory()
{
  {
    EZ_LOCK(m_ReaderWriterMutex);
    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    {
      EZ_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
    }

    for (ezUInt32 i = 0; i < m_Readers.GetCount(); ++i)
    {
      EZ_DEFAULT_DELETE(m_Readers[i]);
    }

    m_Readers.Clear();
  }

  ezDataDirectoryTypeWeb* pThis = this;
  EZ_DEFAULT_DELETE(pThis);
}

bool ezDataDirectoryTypeWeb::ExistsFile(ezStringView sFile, bool bOneSpecificDataDir)
{
  EZ_IGNORE_UNUSED(bOneSpecificDataDir);

  ezStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
  {
    // assume it exists, when it is listed in the redirection table
    return true;
  }

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (ezConversionUtils::IsStringUuid(sRedirected))
    return false;

  // TODO Web: ezDataDirectoryTypeWeb::ExistsFile

  return false;
}

ezDataDirectoryType* ezDataDirectoryTypeWeb::Factory(ezStringView sDataDirectory, ezStringView sGroup, ezStringView sRootName, ezDataDirUsage usage)
{
  EZ_IGNORE_UNUSED(sGroup);
  EZ_IGNORE_UNUSED(sRootName);

  ezStringBuilder sRedirected;
  if (ezFileSystem::ResolveSpecialDirectory(sDataDirectory, sRedirected).Failed())
  {
    sRedirected = sDataDirectory;
  }

  // use the 'web:' prefix to indicate that you want a web server connection
  if (!sRedirected.TrimWordStart("web:"))
    return nullptr;

  if (usage != ezDataDirUsage::ReadOnly)
  {
    ezLog::Error("Can't mount 'web' data directory '{}' ('{}') for writing.", sDataDirectory, sRootName);
    return nullptr;
  }

  ezDataDirectoryTypeWeb* pDataDir = EZ_DEFAULT_NEW(ezDataDirectoryTypeWeb);

  if (pDataDir->InitializeDataDirectory(sRedirected) == EZ_SUCCESS)
    return pDataDir;

  EZ_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

void ezDataDirectoryTypeWeb::OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed)
{
  EZ_LOCK(m_ReaderWriterMutex);
  if (pClosed->IsReader())
  {
    ezDataDirectoryReaderWeb* pReader = (ezDataDirectoryReaderWeb*)pClosed;
    pReader->m_bIsInUse = false;
  }
}

//////////////////////////////////////////////////////////////////////////

ezDataDirectoryReaderWeb::ezDataDirectoryReaderWeb()
  : ezDataDirectoryReader(0)
{
}

ezUInt64 ezDataDirectoryReaderWeb::Read(void* pBuffer, ezUInt64 uiBytes)
{
  const ezUInt64 uiMaxSize = m_Data.GetCount();
  const ezUInt64 uiMaxLeft = uiMaxSize - m_uiReadPosition;
  uiBytes = ezMath::Min(uiBytes, uiMaxLeft);

  ezMemoryUtils::RawByteCopy(pBuffer, ezMemoryUtils::AddByteOffset(m_Data.GetData(), m_uiReadPosition), uiBytes);
  m_uiReadPosition += uiBytes;

  return uiBytes;
}

ezUInt64 ezDataDirectoryReaderWeb::GetFileSize() const
{
  return m_Data.GetCount();
}

ezResult ezDataDirectoryReaderWeb::InternalOpen(ezFileShareMode::Enum FileShareMode)
{
  EZ_IGNORE_UNUSED(FileShareMode);

  ezStringBuilder sPath = ((ezDataDirectoryTypeWeb*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
  sPath.AppendPath(GetFilePath());

  ezResult res = EZ_FAILURE;

#if EZ_ENABLED(EZ_PLATFORM_WEB)
  ezStopwatch sw;

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  ezStringUtils::Copy(attr.requestMethod, 32, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_PERSIST_FILE;

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (ezDataDirectoryTypeWeb::IgnoreClientCache)
  {
    attr.attributes &= ~EMSCRIPTEN_FETCH_PERSIST_FILE;
    attr.attributes |= EMSCRIPTEN_FETCH_REPLACE;
  }
#  endif

  emscripten_fetch_t* fetch = emscripten_fetch(&attr, sPath.GetData());
  if (fetch->status == 200)
  {
    m_Data.SetCountUninitialized(fetch->numBytes);
    ezMemoryUtils::RawByteCopy(m_Data.GetData(), fetch->data, m_Data.GetCount());

    ezLog::Debug("Fetched file: '{}' - {} ({})", fetch->url, ezArgFileSize(fetch->numBytes), sw.GetRunningTotal());
    res = EZ_SUCCESS;
  }
  else
  {
    ezLog::Warning("Failed to fetch file: '{}' - Code {} ({})", fetch->url, fetch->status, sw.GetRunningTotal());
  }
  emscripten_fetch_close(fetch);
#endif

  return res;
}

void ezDataDirectoryReaderWeb::InternalClose()
{
  m_Data.Clear();
  m_uiReadPosition = 0;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeWeb);
