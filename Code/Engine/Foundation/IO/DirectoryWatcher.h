#pragma once

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct ezDirectoryWatcherImpl;

/// \brief Which action has been performed on a file.
enum class ezDirectoryWatcherAction
{
  None,           ///< Nothing happened
  Added,          ///< A file or directory was added, requires ezDirectoryWatcher::Watch::Creates flag when creating ezDirectoryWatcher.
  Removed,        ///< A file or directory was removed, requires ezDirectoryWatcher::Watch::Deletes flag when creating ezDirectoryWatcher.
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file, requires ezDirectoryWatcher::Watch::Writes flag when creating ezDirectoryWatcher.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided, requires ezDirectoryWatcher::Watch::Renames flag when creating ezDirectoryWatcher.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second, requires ezDirectoryWatcher::Watch::Renames flag when creating ezDirectoryWatcher.
};

enum class ezDirectoryWatcherType
{
  File,
  Directory
};

/// \brief Platform-abstracted file system monitoring for detecting directory changes
///
/// Provides efficient monitoring of file system events (create, modify, delete, rename) within
/// a specified directory. Uses native OS facilities (inotify on Linux, ReadDirectoryChangesW on Windows)
/// for minimal overhead polling. Essential for tools like asset hot-reloading, live file editing,
/// and build system dependency tracking.
///
/// Changes are queued internally and retrieved through polling via EnumerateChanges(). Supports
/// configurable event filtering and optional recursive subdirectory monitoring.
class EZ_FOUNDATION_DLL ezDirectoryWatcher
{
public:
  /// \brief What to watch out for.
  struct Watch
  {
    using StorageType = ezUInt8;
    constexpr static ezUInt8 Default = 0;

    /// \brief Enum values
    enum Enum
    {
      Writes = EZ_BIT(0),         ///< Watch for writes. Will trigger ezDirectoryWatcherAction::Modified events.
      Creates = EZ_BIT(1),        ///< Watch for newly created files. Will trigger ezDirectoryWatcherAction::Added events.
      Deletes = EZ_BIT(2),        ///< Watch for deleted files. Will trigger ezDirectoryWatcherAction::Removed events.
      Renames = EZ_BIT(3),        ///< Watch for renames. Will trigger ezDirectoryWatcherAction::RenamedOldName and ezDirectoryWatcherAction::RenamedNewName events.
      Subdirectories = EZ_BIT(4), ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Writes : 1;
      StorageType Creates : 1;
      StorageType Deletes : 1;
      StorageType Renames : 1;
      StorageType Subdirectories : 1;
    };
  };

  ezDirectoryWatcher();
  ezDirectoryWatcher(const ezDirectoryWatcher&) = delete;
  ezDirectoryWatcher(ezDirectoryWatcher&&) noexcept = delete;
  ~ezDirectoryWatcher();

  ezDirectoryWatcher& operator=(const ezDirectoryWatcher&) = delete;
  ezDirectoryWatcher& operator=(ezDirectoryWatcher&&) noexcept = delete;

  /// \brief
  ///   Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of ezDirectoryWatcher can only watch one directory at a time.
  ezResult OpenDirectory(ezStringView sAbsolutePath, ezBitflags<Watch> whatToWatch);

  /// \brief
  ///   Closes the currently watched directory if any.
  void CloseDirectory();

  /// \brief
  ///   Returns the opened directory, will be empty if no directory was opened.
  ezStringView GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = ezDelegate<void(ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type), 48>;

  /// \brief
  ///   Calls the callback \p func for each change since the last call. For each change the filename
  ///   and the action, which was performed on the file, is passed to \p func.
  ///   If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, ezTime waitUpTo = ezTime::MakeZero());

  /// \brief
  ///   Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(ezArrayPtr<ezDirectoryWatcher*> watchers, EnumerateChangesFunction func, ezTime waitUpTo = ezTime::MakeZero());

private:
  ezString m_sDirectoryPath;
  ezDirectoryWatcherImpl* m_pImpl = nullptr;
};

EZ_DECLARE_FLAGS_OPERATORS(ezDirectoryWatcher::Watch);

#endif
