#include <Foundation/PCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FileSystem)

  ON_CORE_STARTUP
  {
    ezFileSystem::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFileSystem::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

ezFileSystem::FileSystemData* ezFileSystem::s_Data = nullptr;

ezFileSystem::FileEvent::FileEvent()
{
  m_EventType = FileEventType::None;
  m_szFileOrDirectory = nullptr;
  m_szOther = nullptr;
  m_pDataDir = nullptr;
}

ezMutex& ezFileSystem::GetFileSystemMutex()
{
  return s_Data->m_Mutex;
}

void ezFileSystem::RegisterEventHandler(ezEvent<const FileEvent&>::Handler handler)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  s_Data->m_Event.AddEventHandler(handler);
}

void ezFileSystem::UnregisterEventHandler(ezEvent<const FileEvent&>::Handler handler)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  s_Data->m_Event.RemoveEventHandler(handler);
}

ezResult ezFileSystem::AddDataDirectory(const char* szDataDirectory, DataDirUsage Usage, const char* szGroup, const char* szCategory)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezStringBuilder sPath = szDataDirectory;
  sPath.MakeCleanPath();

  // use the factory that was added last as the one with the highest priority -> allows to override already added factories
  for (ezInt32 i = s_Data->m_DataDirFactories.GetCount() - 1; i >= 0; --i)
  {
    ezDataDirectoryType* pDataDir = s_Data->m_DataDirFactories[i](sPath.GetData());

    if (pDataDir != nullptr)
    {
      DataDirectory dd;
      dd.m_Usage = Usage;
      dd.m_pDataDirectory = pDataDir;
      dd.m_sCategory = szCategory;
      dd.m_sGroup = szGroup;

      s_Data->m_DataDirectories.PushBack(dd);

      {
        // Broadcast that a data directory was added
        FileEvent fe;
        fe.m_EventType = FileEventType::AddDataDirectorySucceeded;
        fe.m_szFileOrDirectory = sPath.GetData();
        fe.m_szOther = szCategory;
        fe.m_pDataDir = pDataDir;
        s_Data->m_Event.Broadcast(fe);
      }

      return EZ_SUCCESS;
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType = FileEventType::AddDataDirectoryFailed;
    fe.m_szFileOrDirectory = sPath.GetData();
    fe.m_szOther = szCategory;
    s_Data->m_Event.Broadcast(fe);
  }

  ezLog::Error("Adding Data Directory '%s' failed.", szDataDirectory);
  return EZ_FAILURE;
}

ezUInt32 ezFileSystem::RemoveDataDirectoryGroup(const char* szGroup)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezUInt32 uiRemoved = 0;

  for (ezUInt32 i = 0; i < s_Data->m_DataDirectories.GetCount(); )
  {
    if (s_Data->m_DataDirectories[i].m_sGroup == szGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_szFileOrDirectory = s_Data->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath().GetData();
        fe.m_szOther = s_Data->m_DataDirectories[i].m_sCategory.GetData();
        s_Data->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_Data->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
      s_Data->m_DataDirectories.RemoveAt(i);
    }
    else
      ++i;
  }

  return uiRemoved;
}

void ezFileSystem::ClearAllDataDirectories()
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  for (ezInt32 i = s_Data->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    {
      // Broadcast that a data directory is about to be removed
      FileEvent fe;
      fe.m_EventType = FileEventType::RemoveDataDirectory;
      fe.m_szFileOrDirectory = s_Data->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath().GetData();
      fe.m_szOther = s_Data->m_DataDirectories[i].m_sCategory.GetData();
      s_Data->m_Event.Broadcast(fe);
    }

    s_Data->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
  }

  s_Data->m_DataDirectories.Clear();
}

ezUInt32 ezFileSystem::GetNumDataDirectories()
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  return s_Data->m_DataDirectories.GetCount();
}

ezDataDirectoryType* ezFileSystem::GetDataDirectory(ezUInt32 uiDataDirIndex)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  return s_Data->m_DataDirectories[uiDataDirIndex].m_pDataDirectory;
}

const char* ezFileSystem::GetDataDirRelativePath(const char* szPath, ezUInt32 uiDataDir)
{
  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with
  const ezString128& sDirPath = s_Data->m_DataDirectories[uiDataDir].m_pDataDirectory->GetDataDirectoryPath();
    
  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (sDirPath.IsEmpty())
    return szPath;
    
  const char* szRelPath = szPath;

  if (ezStringUtils::StartsWith_NoCase(szPath, sDirPath.GetData()))
    szRelPath = &szPath[sDirPath.GetElementCount()];

  // if the relative path still starts with a path-separator, skip it
  if (ezPathUtils::IsPathSeparator(*szRelPath))
    ++szRelPath;

  return szRelPath;
}

void ezFileSystem::DeleteFile(const char* szFile)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezHybridArray<ezString, 4> SearchDirs;
  szFile = ExtractDataDirsToSearch(szFile, SearchDirs);

  for (ezUInt32 uiSearchDir = 0; uiSearchDir < SearchDirs.GetCount(); ++uiSearchDir)
  {
    for (ezUInt32 i = 0; i < s_Data->m_DataDirectories.GetCount(); ++i)
    {
      // do not delete data from directories that are mounted as read only
      if (s_Data->m_DataDirectories[i].m_Usage != AllowWrites)
        continue;

      if (s_Data->m_DataDirectories[i].m_sCategory != SearchDirs[uiSearchDir])
        continue;

      const char* szRelPath = GetDataDirRelativePath(szFile, i);

      {
        // Broadcast that a file is about to be deleted
        // This can be used to check out files or mark them as deleted in a revision control system
        FileEvent fe;
        fe.m_EventType = FileEventType::DeleteFile;
        fe.m_szFileOrDirectory = szRelPath;
        fe.m_pDataDir = s_Data->m_DataDirectories[i].m_pDataDirectory;
        s_Data->m_Event.Broadcast(fe);
      }
      
      s_Data->m_DataDirectories[i].m_pDataDirectory->DeleteFile(szRelPath);
    }
  }
}

bool ezFileSystem::ExistsFile(const char* szFile)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezHybridArray<ezString, 4> SearchDirs;
  szFile = ExtractDataDirsToSearch(szFile, SearchDirs);

  for (ezUInt32 uiSearchDir = 0; uiSearchDir < SearchDirs.GetCount(); ++uiSearchDir)
  {
    for (ezUInt32 i = 0; i < s_Data->m_DataDirectories.GetCount(); ++i)
    {
      if (s_Data->m_DataDirectories[i].m_sCategory != SearchDirs[uiSearchDir])
        continue;

      const char* szRelPath = GetDataDirRelativePath(szFile, i);
      
      if (s_Data->m_DataDirectories[i].m_pDataDirectory->ExistsFile(szRelPath))
        return true;
    }
  }

  return false;
}

const char* ezFileSystem::ExtractDataDirsToSearch(const char* szPath, ezHybridArray<ezString, 4>& SearchDirs)
{
  // this function searches a path for something like this:
  // "<BIN|DATA|PACKAGE>My/Path/To/A/File.txt"
  // It will extract 'BIN', 'DATA' and 'PACKAGE' and return that in 'SearchDirs'
  // It will also return the const char* to "My/Path/To/A/File.txt"

  SearchDirs.Clear();

  if (!ezStringUtils::StartsWith(szPath, "<"))
  {
    // if the path does not contain a <STUFF> filter, it is as if there is the <> filter (all data dirs with the empty category name)
    SearchDirs.PushBack("");
    return szPath;
  }

  ezStringBuilder sCur;
  ezStringView it(szPath);
  ++it;

  while (!it.IsEmpty() && (it.GetCharacter() != '>'))
  {
    if (it.GetCharacter() == '|')
    {
      SearchDirs.PushBack(sCur);
      sCur.Clear();
    }
    else
      sCur.Append(it.GetCharacter());

    ++it;
  }

  EZ_ASSERT_DEV(!it.IsEmpty(), "Cannot parse the path \"%s\". The data-dir filter starts with a '<' but does not end with '>'.", szPath);

  SearchDirs.PushBack(sCur);
  ++it;

  return it.GetData(); // return the string after the data-dir filter declaration
}

ezDataDirectoryReader* ezFileSystem::GetFileReader(const char* szFile, bool bAllowFileEvents)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezHybridArray<ezString, 4> SearchDirs;
  szFile = ExtractDataDirsToSearch(szFile, SearchDirs);

  // clean up the path to get rid of ".." etc.
  ezStringBuilder sPath = szFile;
  sPath.MakeCleanPath();

  // Go through all Data Directory Categories and let each data dir from each requested Category try to open the file.
  for (ezUInt32 sd = 0; sd < SearchDirs.GetCount(); ++sd)
  {
    // the last added data directory has the highest priority
    for (ezInt32 i = (ezInt32) s_Data->m_DataDirectories.GetCount() - 1; i >= 0; --i)
    {
      // ignore all directories that have not the category that is currently requested
      if (s_Data->m_DataDirectories[i].m_sCategory != SearchDirs[sd])
        continue;

      const char* szRelPath = GetDataDirRelativePath(sPath.GetData(), i);

      if (bAllowFileEvents)
      {
        // Broadcast that we now try to open this file
        // Could be useful to check this file out before it is accessed
        FileEvent fe;
        fe.m_EventType = FileEventType::OpenFileAttempt;
        fe.m_szFileOrDirectory = szRelPath;
        fe.m_szOther = SearchDirs[sd].GetData();
        fe.m_pDataDir = s_Data->m_DataDirectories[i].m_pDataDirectory;
        s_Data->m_Event.Broadcast(fe);
      }

      // Let the data directory try to open the file.
      ezDataDirectoryReader* pReader = s_Data->m_DataDirectories[i].m_pDataDirectory->OpenFileToRead(szRelPath);

      if (bAllowFileEvents && pReader != nullptr)
      {
        // Broadcast that this file has been opened.
        FileEvent fe;
        fe.m_EventType = FileEventType::OpenFileSucceeded;
        fe.m_szFileOrDirectory = szRelPath;
        fe.m_szOther = SearchDirs[sd].GetData();
        fe.m_pDataDir = s_Data->m_DataDirectories[i].m_pDataDirectory;
        s_Data->m_Event.Broadcast(fe);

        return pReader;
      }
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::OpenFileFailed;
    fe.m_szFileOrDirectory = sPath.GetData();
    s_Data->m_Event.Broadcast(fe);
  }

  return nullptr;
}

ezDataDirectoryWriter* ezFileSystem::GetFileWriter(const char* szFile, bool bAllowFileEvents)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezHybridArray<ezString, 4> SearchDirs;
  szFile = ExtractDataDirsToSearch(szFile, SearchDirs);

  // clean up the path to get rid of ".." etc.
  ezStringBuilder sPath = szFile;
  sPath.MakeCleanPath();

  // Go through all Data Directory Categories and let each data dir from each requested Category try to open the file.
  for (ezUInt32 sd = 0; sd < SearchDirs.GetCount(); ++sd)
  {
    // the last added data directory has the highest priority
    for (ezInt32 i = (ezInt32) s_Data->m_DataDirectories.GetCount() - 1; i >= 0; --i)
    {
      if (s_Data->m_DataDirectories[i].m_Usage != AllowWrites)
        continue;

      // ignore all directories that have not the category that is currently requested
      if (s_Data->m_DataDirectories[i].m_sCategory != SearchDirs[sd])
        continue;

      const char* szRelPath = GetDataDirRelativePath(szFile, i);

      if (bAllowFileEvents)
      {
        // Broadcast that we now try to open this file
        // Could be useful to check this file out before it is accessed
        FileEvent fe;
        fe.m_EventType = FileEventType::CreateFileAttempt;
        fe.m_szFileOrDirectory = szRelPath;
        fe.m_szOther = SearchDirs[sd].GetData();
        fe.m_pDataDir = s_Data->m_DataDirectories[i].m_pDataDirectory;
        s_Data->m_Event.Broadcast(fe);
      }

      ezDataDirectoryWriter* pWriter = s_Data->m_DataDirectories[i].m_pDataDirectory->OpenFileToWrite(szRelPath);

      if (bAllowFileEvents && pWriter != nullptr)
      {
        // Broadcast that this file has been created.
        FileEvent fe;
        fe.m_EventType = FileEventType::CreateFileSucceeded;
        fe.m_szFileOrDirectory = szRelPath;
        fe.m_szOther = SearchDirs[sd].GetData();
        fe.m_pDataDir = s_Data->m_DataDirectories[i].m_pDataDirectory;
        s_Data->m_Event.Broadcast(fe);

        return pWriter;
      }
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::CreateFileFailed;
    fe.m_szFileOrDirectory = sPath.GetData();
    s_Data->m_Event.Broadcast(fe);
  }

  return nullptr;
}

ezResult ezFileSystem::ResolvePath(const char* szPath, bool bForWriting, ezString* out_sAbsolutePath, ezString* out_sDataDirRelativePath)
{
  EZ_ASSERT_DEV(s_Data != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_Data->m_Mutex);

  ezResult bRet = EZ_FAILURE;
  bool bWrite = bForWriting;

  // if we only want to read the file, first actually try to read the file
  if (!bWrite)
  {
    // try to get a reader -> if we get one, the file does indeed exist
    ezDataDirectoryReader* pReader = ezFileSystem::GetFileReader(szPath, true);
  
    if (pReader)
    {
      if (out_sAbsolutePath)
      {
        ezStringBuilder sAbs = pReader->GetDataDirectory()->GetDataDirectoryPath();
        sAbs.AppendPath(pReader->GetFilePath().GetData());

        *out_sAbsolutePath = sAbs;
      }

      if (out_sDataDirRelativePath)
        *out_sDataDirRelativePath = pReader->GetFilePath().GetData();

      bRet = EZ_SUCCESS;

      pReader->Close();
    }
    else
    {
      // if we could not get a reader, that means the file does not exist (yet)
      // however, we do want to have some kind of valid path, so next we try to write the file, and check where we end up
      bWrite = true;
    }
  }

  if (bWrite)
  {
    // store the filename and extension for later reuse
    const ezString sFileNameAndExt = ezPathUtils::GetFileNameAndExtension(szPath);

    ezStringBuilder sNewPath = szPath;
    sNewPath.ChangeFileNameAndExtension("__dummy__.tmp"); // we just assume there is no such file

    // note that this also allows to get the path for directories
    // if a path of "Path/To/Folder/" is passed, the dummy file will be "Path/To/Folder/__dummy__.tmp"
    // that will actually create the directory structure (if it did not exist yet)
    // later the "Filename and Extension" will be changed back to "" and thus the path to the folder is returned

    ezDataDirectoryWriter* pWriter = GetFileWriter(sNewPath.GetData(), true); // try to open the file for writing

    if (pWriter)
    {
      ezStringBuilder sRelativePath = pWriter->GetFilePath();    // get the path of the actually opened file
      pWriter->Close();

      ezStringBuilder sAbsPath = pWriter->GetDataDirectory()->GetDataDirectoryPath();
      sAbsPath.AppendPath(sRelativePath.GetData());

      ezFileSystem::DeleteFile(sAbsPath.GetData()); // we use the absolute path here, so this will delete exactly the one file

      if (out_sAbsolutePath)
      {
        sAbsPath.ChangeFileNameAndExtension(sFileNameAndExt.GetData()); // change the filename back to the original filename
        *out_sAbsolutePath = sAbsPath;
      }

      if (out_sDataDirRelativePath)
      {
        sRelativePath.ChangeFileNameAndExtension(sFileNameAndExt.GetData()); // change the filename back to the original filename
        *out_sDataDirRelativePath = sRelativePath;
      }

      bRet = EZ_SUCCESS;
    }
  }

  return bRet;
}

void ezFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  EZ_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  for (auto& dd : s_Data->m_DataDirectories)
  {
    dd.m_pDataDirectory->ReloadExternalConfigs();
  }
}

void ezFileSystem::Startup()
{
  s_Data = EZ_DEFAULT_NEW(FileSystemData);
}

void ezFileSystem::Shutdown()
{
  s_Data->m_DataDirFactories.Clear();
    
  ClearAllDataDirectories();

  EZ_DEFAULT_DELETE(s_Data);
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);

