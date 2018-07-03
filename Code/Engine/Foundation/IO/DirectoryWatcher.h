#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Delegate.h>

struct ezDirectoryWatcherImpl;

/// \brief Which action has been performed on a file.
enum class ezDirectoryWatcherAction
{
  Added,
  Removed,
  Modified,
  RenamedOldName,
  RenamedNewName,
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

    /// \brief Enum values
    enum Enum
    {
      Reads = EZ_BIT(0),         ///< Watch for reads.
      Writes = EZ_BIT(1),        ///< Watch for writes.
      Creates = EZ_BIT(2),       ///< Watch for newly created files.
      Renames = EZ_BIT(3),       ///< Watch for renames.
      Subdirectories = EZ_BIT(4) ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Reads : 1;
      StorageType Writes : 1;
      StorageType Creates : 1;
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

  /// \brief
  ///   Calls the callback \p func for each change since the last call. For each change the filename
  ///   and the action, which was performed on the file, is passed to \p func.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(ezDelegate<void(const char* filename, ezDirectoryWatcherAction action)> func);

private:
  ezString m_sDirectoryPath;
  ezDirectoryWatcherImpl* m_pImpl;
};

EZ_DECLARE_FLAGS_OPERATORS(ezDirectoryWatcher::Watch);
