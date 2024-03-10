#include <ToolsFoundation/ToolsFoundationDLL.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)

#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Threading/DelegateTask.h>

////////////////////////////////////////////////////////////////////////
// ezAssetWatcher
////////////////////////////////////////////////////////////////////////

ezFileSystemWatcher::ezFileSystemWatcher(const ezApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
}


ezFileSystemWatcher::~ezFileSystemWatcher() = default;

void ezFileSystemWatcher::Initialize()
{
  EZ_PROFILE_SCOPE("Initialize");

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    ezStringBuilder sTemp;
    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      ezLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    ezDirectoryWatcher* pWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);
    ezResult res = pWatcher->OpenDirectory(sTemp, ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      EZ_DEFAULT_DELETE(pWatcher);
      ezLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Watcher Changes", ezTaskNesting::Never, [this]()
    {
      ezHybridArray<WatcherResult, 16> watcherResults;
      for (ezDirectoryWatcher* pWatcher : m_Watchers)
      {
        pWatcher->EnumerateChanges([pWatcher, &watcherResults](ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type)
          { watcherResults.PushBack({sFilename, action, type}); });
      }
      for (const WatcherResult& res : watcherResults)
      {
        HandleWatcherChange(res);
      } //
    });
  // This is a separate task as these trigger callbacks which can potentially take a long time and we can't have the watcher changes task be blocked for so long or notifications might get lost.
  m_pNotifyTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Watcher Notify", ezTaskNesting::Never, [this]()
    { NotifyChanges(); });
}

void ezFileSystemWatcher::Deinitialize()
{
  m_bShutdown = true;
  ezTaskGroupID watcherGroup;
  ezTaskGroupID notifyGroup;
  {
    EZ_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
    notifyGroup = m_NotifyGroup;
  }
  ezTaskSystem::WaitForGroup(watcherGroup);
  ezTaskSystem::WaitForGroup(notifyGroup);
  {
    EZ_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    m_pNotifyTask.Clear();
    for (ezDirectoryWatcher* pWatcher : m_Watchers)
    {
      EZ_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }
}

void ezFileSystemWatcher::MainThreadTick()
{
  EZ_PROFILE_SCOPE("ezAssetWatcherTick");
  EZ_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && ezTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = ezTaskSystem::StartSingleTask(m_pWatcherTask, ezTaskPriority::LongRunningHighPriority);
  }
  if (!m_bShutdown && m_pNotifyTask && ezTaskSystem::IsTaskGroupFinished(m_NotifyGroup))
  {
    m_NotifyGroup = ezTaskSystem::StartSingleTask(m_pNotifyTask, ezTaskPriority::LongRunningHighPriority);
  }
}


void ezFileSystemWatcher::NotifyChanges()
{
  auto NotifyChange = [this](const ezString& sAbsPath, ezFileSystemWatcherEvent::Type type)
  {
    ezFileSystemWatcherEvent e;
    e.m_sPath = sAbsPath;
    e.m_Type = type;
    m_Events.Broadcast(e);
  };

  // Files
  ConsumeEntry(m_FileAdded, ezFileSystemWatcherEvent::Type::FileAdded, NotifyChange);
  ConsumeEntry(m_FileChanged, ezFileSystemWatcherEvent::Type::FileChanged, NotifyChange);
  ConsumeEntry(m_FileRemoved, ezFileSystemWatcherEvent::Type::FileRemoved, NotifyChange);

  // Directories
  ConsumeEntry(m_DirectoryAdded, ezFileSystemWatcherEvent::Type::DirectoryAdded, NotifyChange);
  ConsumeEntry(m_DirectoryRemoved, ezFileSystemWatcherEvent::Type::DirectoryRemoved, NotifyChange);
}

void ezFileSystemWatcher::HandleWatcherChange(const WatcherResult& res)
{
  switch (res.m_Action)
  {
    case ezDirectoryWatcherAction::None:
      EZ_ASSERT_DEV(false, "None event should never happen");
      break;
    case ezDirectoryWatcherAction::RenamedNewName:
    case ezDirectoryWatcherAction::Added:
    {
      if (res.m_Type == ezDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryAdded, res.m_sFile, s_AddedFrameDelay);
      }
      else
      {
        AddEntry(m_FileAdded, res.m_sFile, s_AddedFrameDelay);
      }
    }
    break;
    case ezDirectoryWatcherAction::RenamedOldName:
    case ezDirectoryWatcherAction::Removed:
    {
      if (res.m_Type == ezDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
    }
    break;
    case ezDirectoryWatcherAction::Modified:
    {
      if (res.m_Type == ezDirectoryWatcherType::Directory)
      {
        // Can a directory even be modified? In any case, we ignore this change.
        // UpdateEntry(m_DirectoryRemoved, res.sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileChanged, res.m_sFile, s_ModifiedFrameDelay);
      }
    }
    break;
  }
}

void ezFileSystemWatcher::AddEntry(ezDynamicArray<PendingUpdate>& container, const ezStringView sAbsPath, ezUInt32 uiFrameDelay)
{
  EZ_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : container)
  {
    if (update.m_sAbsPath == sAbsPath)
    {
      update.m_uiFrameDelay = uiFrameDelay;
      return;
    }
  }
  PendingUpdate& update = container.ExpandAndGetRef();
  update.m_uiFrameDelay = uiFrameDelay;
  update.m_sAbsPath = sAbsPath;
}

void ezFileSystemWatcher::ConsumeEntry(ezDynamicArray<PendingUpdate>& container, ezFileSystemWatcherEvent::Type type, const ezDelegate<void(const ezString& sAbsPath, ezFileSystemWatcherEvent::Type type)>& consume)
{
  ezHybridArray<PendingUpdate, 16> updates;
  {
    EZ_LOCK(m_WatcherMutex);
    for (ezUInt32 i = container.GetCount(); i > 0; --i)
    {
      PendingUpdate& update = container[i - 1];
      --update.m_uiFrameDelay;
      if (update.m_uiFrameDelay == 0)
      {
        updates.PushBack(update);
        container.RemoveAtAndSwap(i - 1);
      }
    }
  }
  for (const PendingUpdate& update : updates)
  {
    consume(update.m_sAbsPath, type);
  }
}

#endif
