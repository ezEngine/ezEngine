#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Threading/TaskSystem.h>

class ezTask;

/// \brief Event fired by ezFileSystemWatcher::m_Events.
struct ezFileSystemWatcherEvent
{
  enum class Type
  {
    FileAdded,
    FileRemoved,
    FileChanged,
    DirectoryAdded,
    DirectoryRemoved,
  };

  ezStringView m_sPath;
  Type m_Type;
};

/// \brief Creates a file system watcher for the given filesystem config and fires any changes on a worker task via an event.
class EZ_TOOLSFOUNDATION_DLL ezFileSystemWatcher
{
public:
  ezFileSystemWatcher(const ezApplicationFileSystemConfig& fileSystemConfig);
  ~ezFileSystemWatcher();

  /// \brief Once called, file system watchers are created for each data directory and changes are observed.
  void Initialize();

  /// \brief Waits for all pending tasks to complete and then stops observing changes and destroys file system watchers.
  void Deinitialize();

  /// \brief Needs to be called at regular intervals (e.g. each frame) to restart background tasks.
  void MainThreadTick();

public:
  ezEvent<const ezFileSystemWatcherEvent&, ezMutex> m_Events;

private:
  static constexpr ezUInt32 s_AddedFrameDelay = 5;
  static constexpr ezUInt32 s_RemovedFrameDelay = 10;

  struct WatcherResult
  {
    ezString sFile;
    ezDirectoryWatcherAction action;
    ezDirectoryWatcherType type;
  };

  struct PendingUpdate
  {
    ezString sAbsPath;
    ezUInt32 m_uiFrameDelay = 0;
  };

  /// \brief Handles a single change notification by a directory watcher.
  void HandleWatcherChange(const WatcherResult& res);
  /// \brief Handles update delays to allow compacting multiple changes.
  void NotifyChanges();
  /// \brief Adds a change with the given delay to the container. If the entry is already present, only its delay is increased.
  void AddEntry(ezDynamicArray<PendingUpdate>& container, const char* szAbsPath, ezUInt32 uiFrameDelay);
  /// \brief Reduces the delay counter of every item in the container. If a delay reaches zero, it is removed and the callback is fired.
  void ConsumeEntry(ezDynamicArray<PendingUpdate>& container, ezFileSystemWatcherEvent::Type type, const ezDelegate<void(const ezString& sAbsPath, ezFileSystemWatcherEvent::Type type)>& consume);

private:
  // Immutable data after StartInitialize
  ezApplicationFileSystemConfig m_FileSystemConfig;

  // Watchers
  mutable ezMutex m_WatcherMutex;
  ezHybridArray<ezDirectoryWatcher*, 6> m_Watchers;
  ezSharedPtr<ezTask> m_pWatcherTask;
  ezSharedPtr<ezTask> m_pNotifyTask;
  ezTaskGroupID m_WatcherGroup;
  ezTaskGroupID m_NotifyGroup;
  ezAtomicBool m_bShutdown = false;

  // Pending ops
  ezHybridArray<PendingUpdate, 4> m_FileAdded;
  ezHybridArray<PendingUpdate, 4> m_FileRemoved;
  ezHybridArray<PendingUpdate, 4> m_FileChanged;
  ezHybridArray<PendingUpdate, 4> m_DirectoryAdded;
  ezHybridArray<PendingUpdate, 4> m_DirectoryRemoved;
};
