#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/FileSystemConfig.h>

struct ezAssetCuratorEvent;
class ezTask;
struct ezAssetInfo;

/// \brief Creates a file system watcher for the given filesystem config and informs the ezAssetCurator
/// of any changes.
class EZ_EDITORFRAMEWORK_DLL ezAssetWatcher
{
public:
  ezAssetWatcher(const ezApplicationFileSystemConfig& fileSystemConfig);
  ~ezAssetWatcher();

  /// \brief Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

private:
  friend class ezDirectoryUpdateTask;
  friend class ezAssetCurator;
  struct WatcherResult
  {
    ezString sFile;
    ezDirectoryWatcherAction action;
  };

  static constexpr ezUInt32 s_FrameDelay = 5;
  struct PendingUpdate
  {
    ezString sAbsPath;
    ezUInt32 m_uiFrameDelay = s_FrameDelay;
  };


  void HandleWatcherChange(const WatcherResult& res);
  void UpdateFile(const char* szAbsPath);
  void UpdateDirectory(const char* szAbsPath);

private:
  mutable ezMutex m_WatcherMutex;
  ezHybridArray<ezDirectoryWatcher*, 6> m_Watchers;
  ezSharedPtr<ezTask> m_pWatcherTask;
  ezTaskGroupID m_WatcherGroup;

  ezHybridArray<ezTaskGroupID, 4> m_DirectoryUpdates;
  ezHybridArray<PendingUpdate, 4> m_UpdateFile;
  ezHybridArray<PendingUpdate, 4> m_UpdateDirectory;

  ezApplicationFileSystemConfig m_FileSystemConfig;
};

/// \brief Task to scan a directory and inform the ezAssetCurator of any changes.
class ezDirectoryUpdateTask final : public ezTask
{
public:
  ezDirectoryUpdateTask(ezAssetWatcher* pWatcher, const char* szFolder);
  ~ezDirectoryUpdateTask();

  ezAssetWatcher* m_pWatcher = nullptr;
  ezString m_sFolder;

private:
  virtual void Execute() override;
};
