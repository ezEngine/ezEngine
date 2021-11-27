#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

////////////////////////////////////////////////////////////////////////
// ezAssetWatcher
////////////////////////////////////////////////////////////////////////

ezAssetWatcher::ezAssetWatcher(const ezApplicationFileSystemConfig& fileSystemConfig)
{
  EZ_PROFILE_SCOPE("ezAssetWatcher");
  m_FileSystemConfig = fileSystemConfig;
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp;
    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      ezLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    ezDirectoryWatcher* pWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);
    ezResult res =
      pWatcher->OpenDirectory(sTemp, ezDirectoryWatcher::Watch::Reads | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Creates |
                                       ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      EZ_DEFAULT_DELETE(pWatcher);
      ezLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Watcher Update", [this]() {
    ezHybridArray<WatcherResult, 16> watcherResults;
    for (ezDirectoryWatcher* pWatcher : m_Watchers)
    {
      pWatcher->EnumerateChanges([pWatcher, &watcherResults](const char* szFilename, ezDirectoryWatcherAction action) {
        ezStringBuilder sTemp = pWatcher->GetDirectory();
        sTemp.AppendPath(szFilename);
        sTemp.MakeCleanPath();

        if (action == ezDirectoryWatcherAction::Modified)
        {
          if (ezOSFile::ExistsDirectory(sTemp))
            return;
        }

        watcherResults.PushBack({sTemp, action});
      });
    }
    for (const WatcherResult& res : watcherResults)
    {
      HandleWatcherChange(res);
    }
  });
}


ezAssetWatcher::~ezAssetWatcher()
{
  m_bShutdown = true;
  ezTaskGroupID watcherGroup;
  {
    EZ_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
  }
  ezTaskSystem::WaitForGroup(watcherGroup);
  {
    EZ_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    for (ezDirectoryWatcher* pWatcher : m_Watchers)
    {
      EZ_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }

  do
  {
    ezTaskGroupID id;
    {
      EZ_LOCK(m_WatcherMutex);
      if (m_DirectoryUpdates.IsEmpty())
        break;
      id = m_DirectoryUpdates.PeekBack();
    }
    ezTaskSystem::WaitForGroup(id);
  } while (true);

  EZ_LOCK(m_WatcherMutex);
  EZ_ASSERT_DEV(m_DirectoryUpdates.IsEmpty(), "All directory updates should have finished.");
}

void ezAssetWatcher::MainThreadTick()
{
  EZ_PROFILE_SCOPE("ezAssetWatcherTick");
  EZ_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && ezTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = ezTaskSystem::StartSingleTask(m_pWatcherTask, ezTaskPriority::LongRunningHighPriority);
  }

  // Files
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  for (ezUInt32 i = m_UpdateFile.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateFile[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0)
    {
      pCurator->NotifyOfFileChange(update.sAbsPath);
      m_UpdateFile.RemoveAtAndSwap(i - 1);
    }
  }

  // Directories
  for (ezUInt32 i = m_UpdateDirectory.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateDirectory[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0 && !m_bShutdown)
    {
      ezSharedPtr<ezTask> pTask = EZ_DEFAULT_NEW(ezDirectoryUpdateTask, this, update.sAbsPath);
      ezTaskGroupID id = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunningHighPriority, [this](ezTaskGroupID id) {
        EZ_LOCK(m_WatcherMutex);
        m_DirectoryUpdates.RemoveAndSwap(id);
      });
      m_DirectoryUpdates.PushBack(id);

      m_UpdateDirectory.RemoveAtAndSwap(i - 1);
    }
  }
}

void ezAssetWatcher::HandleWatcherChange(const WatcherResult& res)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();
  ezFileStats stat;
  ezResult stats = ezOSFile::GetFileStats(res.sFile, stat);
  bool isFileKnown = false;
  {
    EZ_LOCK(pCurator->m_CuratorMutex);
    isFileKnown = pCurator->m_ReferencedFiles.Find(res.sFile).IsValid();
  }

  switch (res.action)
  {
    case ezDirectoryWatcherAction::None:
      EZ_ASSERT_DEV(false, "None event should never happen");
      break;
    case ezDirectoryWatcherAction::Added:
    {
      if (stats == EZ_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          UpdateDirectory(res.sFile);
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
    }
    break;
    case ezDirectoryWatcherAction::Removed:
    {
      if (isFileKnown)
      {
        UpdateFile(res.sFile);
      }
      else
      {
        UpdateDirectory(res.sFile);
      }
    }
    break;
    case ezDirectoryWatcherAction::Modified:
      if (stats == EZ_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          // TODO: Can directories be modified?
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
      break;
    case ezDirectoryWatcherAction::RenamedOldName:
      // Ignore, we scan entire parent dir in the following RenamedNewName event.
      break;
    case ezDirectoryWatcherAction::RenamedNewName:
    {
      // Rescan parent directory.
      // TODO: Renames on root will rescan the entire data dir.
      // However, fixing this would require dynamic recursion if we detect that
      // a folder was actually renamed.
      ezStringBuilder sParentFolder = res.sFile;
      sParentFolder.PathParentDirectory();
      UpdateDirectory(sParentFolder);
    }
    break;
  }
}

void ezAssetWatcher::UpdateFile(const char* szAbsPath)
{
  EZ_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateFile)
  {
    if (update.sAbsPath == szAbsPath)
    {
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateFile.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath = szAbsPath;
}

void ezAssetWatcher::UpdateDirectory(const char* szAbsPath)
{
  ezStringBuilder sAbsPath = szAbsPath;
  EZ_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateDirectory)
  {
    // No need to add this directory if itself or a parent directory is already queued.
    if (sAbsPath.IsPathBelowFolder(update.sAbsPath))
    {
      // Reset delay as we are probably doing a bigger operation right now.
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateDirectory.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath = sAbsPath;
}

////////////////////////////////////////////////////////////////////////
// ezDirectoryUpdateTask
////////////////////////////////////////////////////////////////////////

ezDirectoryUpdateTask::ezDirectoryUpdateTask(ezAssetWatcher* pWatcher, const char* szFolder)
  : m_pWatcher(pWatcher)
  , m_sFolder(szFolder)
{
  ConfigureTask("ezDirectoryUpdateTask", ezTaskNesting::Never);
}

ezDirectoryUpdateTask::~ezDirectoryUpdateTask() {}

void ezDirectoryUpdateTask::Execute()
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();
  ezSet<ezString> previouslyKnownFiles;
  {
    CURATOR_PROFILE("FindReferencedFiles");
    // Find all currently known files that are under the given folder.
    // TODO: is m_InverseDependency / m_InverseReferences covered by this?
    // TODO: What about asset output files?
    EZ_LOCK(pCurator->m_CuratorMutex);
    auto itlowerBound = pCurator->m_ReferencedFiles.LowerBound(m_sFolder);
    while (itlowerBound.IsValid() && itlowerBound.Key().StartsWith_NoCase(m_sFolder))
    {
      previouslyKnownFiles.Insert(itlowerBound.Key());
      ++itlowerBound;
    }
  }

  {
    CURATOR_PROFILE("IterateDataDirectory");
    // Iterate folder to find all actually existing files on disk.
    ezSet<ezString> knownFiles;
    pCurator->IterateDataDirectory(m_sFolder, &knownFiles);

    // Not encountered files are now removed which the ezAssetCurator must be informed about.
    previouslyKnownFiles.Difference(knownFiles);
  }

  CURATOR_PROFILE("HandleRemovedFiles");
  for (const ezString& sFile : previouslyKnownFiles)
  {
    pCurator->HandleSingleFile(sFile);
  }
}
