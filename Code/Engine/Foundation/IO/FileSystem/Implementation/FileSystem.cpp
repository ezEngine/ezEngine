#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringView.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FileSystem)

  ON_CORESYSTEMS_STARTUP
  {
    ezFileSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezFileSystem::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezFileSystem::FileSystemData* ezFileSystem::s_pData = nullptr;
ezString ezFileSystem::s_sSdkRootDir;
ezMap<ezString, ezString> ezFileSystem::s_SpecialDirectories;


void ezFileSystem::RegisterDataDirectoryFactory(ezDataDirFactory factory, float fPriority /*= 0*/)
{
  // This assert helps finding cases where the ezFileSystem is used without ez being properly initialized or already shutdown.
  // The code would crash below anyways but asserts are easier to see in automated testing on e.g. CI.
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_pData->m_FsMutex);

  auto& data = s_pData->m_DataDirFactories.ExpandAndGetRef();
  data.m_Factory = factory;
  data.m_fPriority = fPriority;
}

ezEventSubscriptionID ezFileSystem::RegisterEventHandler(ezEvent<const FileEvent&>::Handler handler)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_Event.AddEventHandler(handler);
}

void ezFileSystem::UnregisterEventHandler(ezEvent<const FileEvent&>::Handler handler)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(handler);
}

void ezFileSystem::UnregisterEventHandler(ezEventSubscriptionID subscriptionId)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(subscriptionId);
}

void ezFileSystem::CleanUpRootName(ezStringBuilder& sRoot)
{
  // this cleaning might actually make the root name empty
  // e.g. ":" becomes ""
  // which is intended to support passing through of absolute paths
  // ie. mounting the empty dir "" under the root ":" will allow to write directly to files using absolute paths

  while (sRoot.StartsWith(":"))
    sRoot.Shrink(1, 0);

  while (sRoot.EndsWith("/"))
    sRoot.Shrink(0, 1);

  sRoot.ToUpper();
}

ezResult ezFileSystem::AddDataDirectory(ezStringView sDataDirectory, ezStringView sGroup, ezStringView sRootName, ezDataDirUsage usage)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_ASSERT_DEV(usage != ezDataDirUsage::AllowWrites || !sRootName.IsEmpty(), "A data directory must have a non-empty, unique name to be mounted for write access");

  ezStringBuilder sPath = sDataDirectory;
  sPath.MakeCleanPath();

  if (!sPath.IsEmpty() && !sPath.EndsWith("/"))
    sPath.Append("/");

  ezStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  EZ_LOCK(s_pData->m_FsMutex);

  bool failed = false;
  if (FindDataDirectoryWithRoot(sCleanRootName) != nullptr)
  {
    ezLog::Error("A data directory with root name '{0}' already exists.", sCleanRootName);
    failed = true;
  }

  if (!failed)
  {
    s_pData->m_DataDirFactories.Sort([](const auto& a, const auto& b)
      { return a.m_fPriority < b.m_fPriority; });

    // use the factory that was added last as the one with the highest priority -> allows to override already added factories
    for (ezInt32 i = s_pData->m_DataDirFactories.GetCount() - 1; i >= 0; --i)
    {
      ezDataDirectoryType* pDataDir = s_pData->m_DataDirFactories[i].m_Factory(sPath, sGroup, sRootName, usage);

      if (pDataDir != nullptr)
      {
        ezDataDirectoryInfo dd;
        dd.m_Usage = usage;
        dd.m_pDataDirType = pDataDir;
        dd.m_sRootName = sCleanRootName;
        dd.m_sGroup = sGroup;

        s_pData->m_DataDirectories.PushBack(dd);

        {
          // Broadcast that a data directory was added
          FileEvent fe;
          fe.m_EventType = FileEventType::AddDataDirectorySucceeded;
          fe.m_sFileOrDirectory = sPath;
          fe.m_sOther = sCleanRootName;
          fe.m_pDataDir = pDataDir;
          s_pData->m_Event.Broadcast(fe);
        }

        return EZ_SUCCESS;
      }
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType = FileEventType::AddDataDirectoryFailed;
    fe.m_sFileOrDirectory = sPath;
    fe.m_sOther = sCleanRootName;
    s_pData->m_Event.Broadcast(fe);
  }

  ezLog::Error("Adding Data Directory '{0}' failed.", ezArgSensitive(sDataDirectory, "Path"));
  return EZ_FAILURE;
}


bool ezFileSystem::RemoveDataDirectory(ezStringView sRootName)
{
  ezStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  for (ezUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    const auto& directory = s_pData->m_DataDirectories[i];

    if (directory.m_sRootName == sCleanRootName)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = directory.m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther = directory.m_sRootName;
        fe.m_pDataDir = directory.m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      directory.m_pDataDirType->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);

      return true;
    }
    else
      ++i;
  }

  return false;
}

ezUInt32 ezFileSystem::RemoveDataDirectoryGroup(ezStringView sGroup)
{
  if (s_pData == nullptr)
    return 0;

  EZ_LOCK(s_pData->m_FsMutex);

  ezUInt32 uiRemoved = 0;

  for (ezUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sGroup == sGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);
    }
    else
      ++i;
  }

  return uiRemoved;
}

void ezFileSystem::ClearAllDataDirectories()
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_pData->m_FsMutex);

  for (ezInt32 i = s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    {
      // Broadcast that a data directory is about to be removed
      FileEvent fe;
      fe.m_EventType = FileEventType::RemoveDataDirectory;
      fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
      fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
  }

  s_pData->m_DataDirectories.Clear();
}

const ezDataDirectoryInfo* ezFileSystem::FindDataDirectoryWithRoot(ezStringView sRootName)
{
  if (sRootName.IsEmpty())
    return nullptr;

  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_sRootName.IsEqual_NoCase(sRootName))
    {
      return &dd;
    }
  }

  return nullptr;
}

ezUInt32 ezFileSystem::GetNumDataDirectories()
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories.GetCount();
}

ezDataDirectoryType* ezFileSystem::GetDataDirectory(ezUInt32 uiDataDirIndex)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex].m_pDataDirType;
}

const ezDataDirectoryInfo& ezFileSystem::GetDataDirectoryInfo(ezUInt32 uiDataDirIndex)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex];
}

ezStringView ezFileSystem::GetDataDirRelativePath(ezStringView sPath, ezUInt32 uiDataDir)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with

  // first check the redirected directory
  const ezString128& sRedDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetRedirectedDataDirectoryPath();

  if (!sRedDirPath.IsEmpty() && sPath.StartsWith_NoCase(sRedDirPath))
  {
    ezStringView sRelPath(sPath.GetStartPointer() + sRedDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (ezPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  // then check the original mount path
  const ezString128& sDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetDataDirectoryPath();

  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (!sDirPath.IsEmpty() && sPath.StartsWith_NoCase(sDirPath))
  {
    ezStringView sRelPath(sPath.GetStartPointer() + sDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (ezPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  return sPath;
}


ezDataDirectoryInfo* ezFileSystem::GetDataDirForRoot(const ezString& sRoot)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sRoot)
      return &s_pData->m_DataDirectories[i];
  }

  return nullptr;
}


void ezFileSystem::DeleteFile(ezStringView sFile)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (ezPathUtils::IsAbsolutePath(sFile))
  {
    ezOSFile::DeleteFile(sFile).IgnoreResult();
    return;
  }

  ezString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  EZ_ASSERT_DEV(!sRootName.IsEmpty(), "Files can only be deleted with a rooted path name.");

  if (sRootName.IsEmpty())
    return;

  EZ_LOCK(s_pData->m_FsMutex);

  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // do not delete data from directories that are mounted as read only
    if (s_pData->m_DataDirectories[i].m_Usage != ezDataDirUsage::AllowWrites)
      continue;

    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    ezStringView sRelPath = GetDataDirRelativePath(sFile, i);

    {
      // Broadcast that a file is about to be deleted
      // This can be used to check out files or mark them as deleted in a revision control system
      FileEvent fe;
      fe.m_EventType = FileEventType::DeleteFile;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      fe.m_sOther = sRootName;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->DeleteFile(sRelPath);
  }
}

bool ezFileSystem::ExistsFile(ezStringView sFile)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  ezString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  EZ_LOCK(s_pData->m_FsMutex);

  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    ezStringView sRelPath = GetDataDirRelativePath(sFile, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->ExistsFile(sRelPath, bOneSpecificDataDir))
      return true;
  }

  return false;
}


ezResult ezFileSystem::GetFileStats(ezStringView sFileOrFolder, ezFileStats& out_stats)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_pData->m_FsMutex);

  if (sFileOrFolder.IsEmpty())
  {
    return EZ_FAILURE;
  }

  ezString sRootName;
  sFileOrFolder = ExtractRootName(sFileOrFolder, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    ezStringView sRelPath = GetDataDirRelativePath(sFileOrFolder, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->GetFileStats(sRelPath, bOneSpecificDataDir, out_stats).Succeeded())
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezStringView ezFileSystem::ExtractRootName(ezStringView sPath, ezString& rootName)
{
  ezStringView root, path;
  ezPathUtils::GetRootedPathParts(sPath, root, path);

  ezStringBuilder rootUpr = root;
  rootUpr.ToUpper();
  rootName = rootUpr;
  return path;
}

ezDataDirectoryReader* ezFileSystem::GetFileReader(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  EZ_LOCK(s_pData->m_FsMutex);

  ezString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  // clean up the path to get rid of ".." etc.
  ezStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  // the last added data directory has the highest priority
  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // if a root is used, ignore all directories that do not have the same root name
    if (bOneSpecificDataDir && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    ezStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::OpenFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    // Let the data directory try to open the file.
    ezDataDirectoryReader* pReader = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToRead(sRelPath, FileShareMode, bOneSpecificDataDir);

    if (bAllowFileEvents && pReader != nullptr)
    {
      // Broadcast that this file has been opened.
      FileEvent fe;
      fe.m_EventType = FileEventType::OpenFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);

      return pReader;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::OpenFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

ezDataDirectoryWriter* ezFileSystem::GetFileWriter(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  EZ_LOCK(s_pData->m_FsMutex);

  ezString sRootName;

  if (!ezPathUtils::IsAbsolutePath(sFile))
  {
    EZ_ASSERT_DEV(sFile.StartsWith(":"),
      "Only native absolute paths or rooted paths (starting with a colon and then the data dir root name) are allowed for "
      "writing to files. This path is neither: '{0}'",
      sFile);
    sFile = ExtractRootName(sFile, sRootName);
  }

  // clean up the path to get rid of ".." etc.
  ezStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  // the last added data directory has the highest priority
  for (ezInt32 i = (ezInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_Usage != ezDataDirUsage::AllowWrites)
      continue;

    // ignore all directories that have not the category that is currently requested
    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    ezStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::CreateFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    ezDataDirectoryWriter* pWriter = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToWrite(sRelPath, FileShareMode);

    if (bAllowFileEvents && pWriter != nullptr)
    {
      // Broadcast that this file has been created.
      FileEvent fe;
      fe.m_EventType = FileEventType::CreateFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);

      return pWriter;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::CreateFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

ezResult ezFileSystem::ResolvePath(ezStringView sPath, ezStringBuilder* out_pAbsolutePath, ezStringBuilder* out_pDataDirRelativePath, const ezDataDirectoryInfo** out_pDataDir /*= nullptr*/)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  EZ_LOCK(s_pData->m_FsMutex);

  ezStringBuilder absPath, relPath;

  if (sPath.StartsWith(":"))
  {
    // writing is only allowed using rooted paths
    ezString sRootName;
    ExtractRootName(sPath, sRootName);

    const ezDataDirectoryInfo* pDataDir = GetDataDirForRoot(sRootName);

    if (pDataDir == nullptr)
      return EZ_FAILURE;

    if (out_pDataDir != nullptr)
      *out_pDataDir = pDataDir;

    relPath = sPath.GetShrunk(sRootName.GetCharacterCount() + 2);

    absPath = pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);
  }
  else if (ezPathUtils::IsAbsolutePath(sPath))
  {
    absPath = sPath;
    absPath.MakeCleanPath();

    for (ezUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
    {
      auto& dir = s_pData->m_DataDirectories[dd - 1];

      if (ezPathUtils::IsSubPath(dir.m_pDataDirType->GetRedirectedDataDirectoryPath(), absPath))
      {
        if (out_pAbsolutePath)
          *out_pAbsolutePath = absPath;

        if (out_pDataDirRelativePath)
        {
          *out_pDataDirRelativePath = absPath;
          out_pDataDirRelativePath->MakeRelativeTo(dir.m_pDataDirType->GetRedirectedDataDirectoryPath()).IgnoreResult();
        }

        if (out_pDataDir)
          *out_pDataDir = &dir;

        return EZ_SUCCESS;
      }
    }

    return EZ_FAILURE;
  }
  else
  {
    // try to get a reader -> if we get one, the file does indeed exist
    ezDataDirectoryReader* pReader = ezFileSystem::GetFileReader(sPath, ezFileShareMode::SharedReads, true);

    if (!pReader)
      return EZ_FAILURE;

    if (out_pDataDir != nullptr)
    {
      for (ezUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
      {
        auto& dir = s_pData->m_DataDirectories[dd - 1];

        if (dir.m_pDataDirType == pReader->GetDataDirectory())
        {
          *out_pDataDir = &dir;
        }
      }
    }

    relPath = pReader->GetFilePath();

    absPath = pReader->GetDataDirectory()->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);

    pReader->Close();
  }

  if (out_pAbsolutePath)
    *out_pAbsolutePath = absPath;

  if (out_pDataDirRelativePath)
    *out_pDataDirRelativePath = relPath;

  return EZ_SUCCESS;
}

ezResult ezFileSystem::FindFolderWithSubPath(ezStringBuilder& out_sResult, ezStringView sStartDirectory, ezStringView sSubPath, ezStringView sRedirectionFileName /*= nullptr*/)
{
  ezStringBuilder sStartDirAbs = sStartDirectory;
  sStartDirAbs.MakeCleanPath();

  // in this case the given path and the absolute path are different
  // but we want to return the same path format as is given
  // ie. if we get ":MyRoot\Bla" with "MyRoot" pointing to "C:\Game", then the result should be
  // ":MyRoot\blub", rather than "C:\Game\blub"
  if (sStartDirAbs.StartsWith(":"))
  {
    ezStringBuilder abs;
    if (ResolvePath(sStartDirAbs, &abs, nullptr).Failed())
    {
      out_sResult.Clear();
      return EZ_FAILURE;
    }

    sStartDirAbs = abs;
  }

  out_sResult = sStartDirectory;
  out_sResult.MakeCleanPath();

  ezStringBuilder FullPath, sRedirection;

  while (!out_sResult.IsEmpty())
  {
    sRedirection.Clear();

    if (!sRedirectionFileName.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirectionFileName);

      ezOSFile f;
      if (f.Open(FullPath, ezFileOpenMode::Read).Succeeded())
      {
        ezDataBuffer db;
        f.ReadAll(db);
        sRedirection.Set(ezStringView((const char*)db.GetData(), db.GetCount()));
      }
    }

    // first try with the redirection
    if (!sRedirection.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirection);
      FullPath.AppendPath(sSubPath);
      FullPath.MakeCleanPath();

      if (ezOSFile::ExistsDirectory(FullPath) || ezOSFile::ExistsFile(FullPath))
      {
        out_sResult.AppendPath(sRedirection);
        out_sResult.MakeCleanPath();
        return EZ_SUCCESS;
      }
    }

    // then try without the redirection
    FullPath = sStartDirAbs;
    FullPath.AppendPath(sSubPath);
    FullPath.MakeCleanPath();

    if (ezOSFile::ExistsDirectory(FullPath) || ezOSFile::ExistsFile(FullPath))
    {
      return EZ_SUCCESS;
    }

    out_sResult.PathParentDirectory();
    sStartDirAbs.PathParentDirectory();
  }

  return EZ_FAILURE;
}

bool ezFileSystem::ResolveAssetRedirection(ezStringView sPathOrAssetGuid, ezStringBuilder& out_sRedirection)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_pDataDirType->ResolveAssetRedirection(sPathOrAssetGuid, out_sRedirection))
      return true;
  }

  out_sRedirection = sPathOrAssetGuid;
  return false;
}

ezStringView ezFileSystem::MigrateFileLocation(ezStringView sOldLocation, ezStringView sNewLocation)
{
  ezStringBuilder sOldPathFull, sNewPathFull;

  if (ResolvePath(sOldLocation, &sOldPathFull, nullptr).Failed() || sOldPathFull.IsEmpty())
  {
    // if the old path could not be resolved, use the new path
    return sNewLocation;
  }

  ResolvePath(sNewLocation, &sNewPathFull, nullptr).AssertSuccess();

  if (!ExistsFile(sOldPathFull))
  {
    // old path doesn't exist -> use the new
    return sNewLocation;
  }

  // old path does exist -> deal with it

  if (ExistsFile(sNewPathFull))
  {
    // new path also exists -> delete the old one (in all data directories), use the new one
    DeleteFile(sOldLocation); // location, not full path
    return sNewLocation;
  }

  // new one doesn't exist -> try to move old to new
  if (ezOSFile::MoveFileOrDirectory(sOldPathFull, sNewPathFull).Failed())
  {
    // if the old location exists, but we can't move the file, return the old location to use
    return sOldLocation;
  }

  // deletes the file in the old location in ALL data directories,
  // so that they can't interfere with the new file in the future
  DeleteFile(sOldLocation); // location, not full path

  // if we successfully moved the file to the new location, use the new location
  return sNewLocation;
}

void ezFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  EZ_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    dd.m_pDataDirType->ReloadExternalConfigs();
  }
}

void ezFileSystem::Startup()
{
  s_pData = EZ_DEFAULT_NEW(FileSystemData);
}

void ezFileSystem::Shutdown()
{
  {
    EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
    EZ_LOCK(s_pData->m_FsMutex);

    s_pData->m_DataDirFactories.Clear();

    ClearAllDataDirectories();
  }

  EZ_DEFAULT_DELETE(s_pData);
}

ezResult ezFileSystem::DetectSdkRootDirectory(ezStringView sExpectedSubFolder /*= "Data/Base"*/)
{
  if (!s_sSdkRootDir.IsEmpty())
    return EZ_SUCCESS;

  ezStringBuilder sdkRoot;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  EZ_IGNORE_UNUSED(sExpectedSubFolder);
  // Probably this is what needs to be done on all mobile platforms as well
  sdkRoot = ezOSFile::GetApplicationDirectory();
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
  sdkRoot = ezOSFile::GetApplicationDirectory();
#else
  if (ezFileSystem::FindFolderWithSubPath(sdkRoot, ezOSFile::GetApplicationDirectory(), sExpectedSubFolder, "ezSdkRoot.txt").Failed())
  {
    ezLog::Error("Could not find SDK root. Application dir is '{0}'. Searched for parent with '{1}' sub-folder.", ezOSFile::GetApplicationDirectory(), sExpectedSubFolder);
    return EZ_FAILURE;
  }
#endif

  ezFileSystem::SetSdkRootDirectory(sdkRoot);
  return EZ_SUCCESS;
}

void ezFileSystem::SetSdkRootDirectory(ezStringView sSdkDir)
{
  ezStringBuilder s = sSdkDir;
  s.MakeCleanPath();

  s_sSdkRootDir = s;
}

ezStringView ezFileSystem::GetSdkRootDirectory()
{
  EZ_ASSERT_DEV(!s_sSdkRootDir.IsEmpty(), "The project directory has not been set through 'ezFileSystem::SetSdkRootDirectory'.");
  return s_sSdkRootDir;
}

void ezFileSystem::SetSpecialDirectory(ezStringView sName, ezStringView sReplacement)
{
  ezStringBuilder tmp = sName;
  tmp.ToLower();

  if (sReplacement.IsEmpty())
  {
    s_SpecialDirectories.Remove(tmp);
  }
  else
  {
    s_SpecialDirectories[tmp] = sReplacement;
  }
}

ezResult ezFileSystem::ResolveSpecialDirectory(ezStringView sDirectory, ezStringBuilder& out_sPath)
{
  if (sDirectory.IsEmpty() || !sDirectory.StartsWith(">"))
  {
    out_sPath = sDirectory;
    return EZ_SUCCESS;
  }

  // skip the '>'
  sDirectory.ChopAwayFirstCharacterAscii();
  const char* szStart = sDirectory.GetStartPointer();

  const char* szEnd = sDirectory.FindSubString("/");

  if (szEnd == nullptr)
    szEnd = szStart + ezStringUtils::GetStringElementCount(szStart);

  ezStringBuilder sName;
  sName.SetSubString_FromTo(szStart, szEnd);
  sName.ToLower();

  const auto it = s_SpecialDirectories.Find(sName);
  if (it.IsValid())
  {
    out_sPath = it.Value();
    out_sPath.AppendPath(szEnd); // szEnd might be on \0 or a slash
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }

  if (sName == "sdk")
  {
    sDirectory.Shrink(3, 0);
    out_sPath = GetSdkRootDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }

  if (sName == "user")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = ezOSFile::GetUserDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }

  if (sName == "temp")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = ezOSFile::GetTempDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }

  if (sName == "appdir")
  {
    sDirectory.Shrink(6, 0);
    out_sPath = ezOSFile::GetApplicationDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}


ezMutex& ezFileSystem::GetMutex()
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  return s_pData->m_FsMutex;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

void ezFileSystem::StartSearch(ezFileSystemIterator& ref_iterator, ezStringView sSearchTerm, ezBitflags<ezFileSystemIteratorFlags> flags /*= ezFileSystemIteratorFlags::Default*/)
{
  EZ_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  EZ_LOCK(s_pData->m_FsMutex);

  ezHybridArray<ezString, 16> folders;
  ezStringBuilder sDdPath, sRelPath;

  if (sSearchTerm.IsRootedPath())
  {
    const ezStringView root = sSearchTerm.GetRootedPathRootName();

    const ezDataDirectoryInfo* pDataDir = FindDataDirectoryWithRoot(root);
    if (pDataDir == nullptr)
      return;

    sSearchTerm.SetStartPosition(root.GetEndPointer());

    if (!sSearchTerm.IsEmpty())
    {
      // root name should be followed by a slash
      sSearchTerm.ChopAwayFirstCharacterAscii();
    }

    folders.PushBack(pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath().GetView());
  }
  else if (sSearchTerm.IsAbsolutePath())
  {
    for (ezUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      sRelPath = sSearchTerm;

      if (!sDdPath.IsEmpty())
      {
        if (sRelPath.MakeRelativeTo(sDdPath).Failed())
          continue;

        // this would use "../" if necessary, which we don't want
        if (sRelPath.StartsWith(".."))
          continue;
      }

      sSearchTerm = sRelPath;

      folders.PushBack(sDdPath);
      break;
    }
  }
  else
  {
    for (ezUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      folders.PushBack(sDdPath);
    }
  }

  ref_iterator.StartMultiFolderSearch(folders, sSearchTerm, flags);
}

#endif

ezResult ezFileSystem::CreateDirectoryStructure(ezStringView sPath)
{
  ezStringBuilder sRedir;
  EZ_SUCCEED_OR_RETURN(ResolveSpecialDirectory(sPath, sRedir));

  if (sRedir.IsRootedPath())
  {
    ezFileSystem::ResolvePath(sRedir, &sRedir, nullptr).AssertSuccess();
  }

  return ezOSFile::CreateDirectoryStructure(sRedir);
}

EZ_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);
