#pragma once

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct ezDirectoryWatcherImpl;

/// \brief Which action has been performed on a file.
enum class ezDirectoryWatcherAction
{
  None,           ///< Nothing happend
  Added,          ///< A file or directory was added
  Removed,        ///< A file or directory was removed
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second.
};

enum class ezDirectoryWatcherType
{
  File,
  Directory
};

/// \brief
///   Watches file actions in a directory. Changes need to be polled.
class EZ_FOUNDATION_DLL ezDirectoryWatcher
{
public:
  /// \brief What to watch out for.
  struct Watch
  {
    typedef ezUInt8 StorageType;
    constexpr static ezUInt8 Default = 0;

    /// \brief Enum values
    enum Enum
    {
      Writes = EZ_BIT(0),         ///< Watch for writes.
      Creates = EZ_BIT(1),        ///< Watch for newly created files.
      Deletes = EZ_BIT(2),        ///< Watch for deleted files.
      Renames = EZ_BIT(3),        ///< Watch for renames.
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
  ~ezDirectoryWatcher();

  /// \brief
  ///   Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of ezDirectoryWatcher can only watch one directory at a time.
  ezResult OpenDirectory(const ezString& absolutePath, ezBitflags<Watch> whatToWatch);

  /// \brief
  ///   Closes the currently watched directory if any.
  void CloseDirectory();

  /// \brief
  ///   Returns the opened directory, will be empty if no directory was opened.
  const char* GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = ezDelegate<void(const char* filename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type), 48>;

  /// \brief
  ///   Calls the callback \p func for each change since the last call. For each change the filename
  ///   and the action, which was performed on the file, is passed to \p func.
  ///   If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, ezTime waitUpTo = ezTime::Zero());

  /// \brief
  ///   Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(ezArrayPtr<ezDirectoryWatcher*> watchers, EnumerateChangesFunction func, ezTime waitUpTo = ezTime::Zero());

private:
  ezString m_sDirectoryPath;
  ezDirectoryWatcherImpl* m_pImpl = nullptr;
};

EZ_DECLARE_FLAGS_OPERATORS(ezDirectoryWatcher::Watch);

#endif
