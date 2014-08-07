#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Threading/Mutex.h>

/// \brief The ezFileSystem provides high-level functionality to manage files in a virtual file system.
///
/// There are two sides at which the file system can be extended:
/// Data directories are the 'sources' of data. These can be simple folders, zip files, data-bases, HTTP servers, etc.
/// Different ezDataDirectoryType's can implement these different 'decoding methods', i.e. they handle how to actually
/// access the data and they use their own readers/writers to implement a common interface for passing data streams
/// to and from the data directory.
/// On the other end there are the actual file readers/writers, which implement policies how to optimize these reads/writes.
/// The default ezFileReader and ezFileWriter implement a buffering policy, i.e. they use an internal cache to only
/// sporadically read or write to the actual data stream.
/// A 'threaded' or 'parallel' file reader/writer could implement a different policy, where a file is read/written
/// in a thread and thus allows to have non-blocking file accesses.
///
/// Which policy to use is defined by the user every time he needs to access a file, by simply using the desired 
/// reader/writer class.
/// How to mount data directories (i.e. with which ezDataDirectoryType) is defined by the 'DataDirFactories', which
/// are functions that create ezDataDirectoryType's. This way one can mount the same data directory (e.g. "MyTestDir")
/// differently, depending on which Factories have been registered previously.
/// This allows to easily configure how to set up data directories.
/// E.g. by default ordinary folders will be mounted to be read from the local file system.
/// However, by registering a different Factory, the same directory could also be mounted over a network on a remote
/// file serving machine.
///
/// Additionally ezFileSystem also broadcasts events about which files are (about to be) accessed.
/// This allows to hook into the system and implement stuff like automatic asset transformations before/after certain
/// file accesses, checking out files from revision control systems, or simply logging all file activity.
///
/// All operations that go through the ezFileSystem are protected by a mutex, which means that opening, closing, deleting
/// files, as well as adding or removing data directories etc. will be synchronized and cannot happen in parallel.
/// Reading/writing file streams can happen in parallel, only the administrative tasks need to be protected.
/// File events are broadcast as they occur, that means they will be executed on whichever thread triggered them.
/// Since they are executed from within the filesystem mutex, they cannot occur in parallel.
class EZ_FOUNDATION_DLL ezFileSystem
{
public:
  /// \brief Enum that describes the type of file-event that occurred.
  struct FileEventType;

  /// \brief The data that is sent through the event interface.
  struct FileEvent;

  /// \brief Registers an Event Handler that will be informed about all the events that the file system broadcasts.
  static void RegisterEventHandler(ezEvent<const FileEvent&>::Handler handler);

  /// \brief Unregisters a previously registered Event Handler.
  static void UnregisterEventHandler(ezEvent<const FileEvent&>::Handler handler);

  /// \brief Returns the mutex that the filesystem uses.
  static ezMutex& GetFileSystemMutex();

public:
  /// \brief This factory creates a data directory type, if it can handle the given data directory. Otherwise it returns nullptr.
  ///
  /// Every time a data directory is supposed to be added, the file system will query its data dir factories, which one
  /// can successfully create an ezDataDirectoryType. In this process the last factory added has the highest priority.
  /// Once a factory is found that was able to create a ezDataDirectoryType, that one is used.
  /// Different factories can be used to mount different types of data directories. But the same directory can also be
  /// mounted in different ways. For example a simple folder could be mounted on the local system, or via a HTTP server
  /// over a network (lets call it a 'FileServer'). Thus depending on which type of factories are registered, the file system
  /// can provide data from very different sources.
  typedef ezDataDirectoryType* (*ezDataDirFactory)(const char* szDataDirectory);

  /// \brief This function allows to register another data directory factory, which might be invoked when a new data directory is to be added.
  static void RegisterDataDirectoryFactory(ezDataDirFactory Factory) { s_Data->m_DataDirFactories.PushBack(Factory); }

  /// \brief Will remove all known data directory factories.
  static void ClearAllDataDirectoryFactories() { s_Data->m_DataDirFactories.Clear(); }

  /// \brief Describes in which mode a data directory is mounted.
  enum DataDirUsage
  {
    ReadOnly,
    AllowWrites,
  };

  /// \brief Adds a data directory. It will try all the registered factories to find a data directory type that can handle the given path.
  ///
  /// If Usage is ReadOnly, writing to the data directory is not allowed. This is independent of whether the data directory type
  /// COULD write anything.
  /// szGroup defines to what 'group' of data directories this data dir belongs. This is only used in calls to RemoveDataDirectoryGroup,
  /// to remove all data directories of the same group.
  /// You could use groups such as 'Base', 'Project', 'Settings', 'Level', 'Temp' to distinguish between different sets of data directories.
  /// You can also specify the exact same string as szDataDirectory for szGroup, and thus uniquely identify the data dir, to be able to remove just that one.
  /// szCategory can be a (short) identifier, such as 'BIN', 'PACKAGE', 'SETTINGS' etc.
  /// Categories are used to look up files in specific data directories via a path-prefix.
  /// For example the path "<BIN>MyFolder/MyFile.txt" will only be looked up in all data directories of the "BIN" category.
  /// The path "MyFolder/MyFile.txt" would only be looked up in folders of the "" (empty) category.
  /// You can specify several search categories in one path by separating them with |
  /// E.g. the path "<BIN|TEMP|>MyFolder/MyFile.txt" would be looked up in all data directories of the category "BIN", "TEMP" and "" (empty)
  /// The path "MyFolder/MyFile.txt" is a shorthand for "<>MyFolder/MyFile.txt" (specifies only the empty category).
  /// If you want a data directory to belong to several different categories, just mount it several times with different categories.
  static ezResult AddDataDirectory(const char* szDataDirectory, DataDirUsage Usage = AllowWrites, const char* szGroup = "", const char* szCategory = "");

  /// \brief Removes all data directories that belong to the given group. Returns the number of data directories that were removed.
  static ezUInt32 RemoveDataDirectoryGroup(const char* szGroup);

  /// \brief Removes all data directories.
  static void ClearAllDataDirectories();

  /// \brief Returns the number of currently active data directories.
  static ezUInt32 GetNumDataDirectories();

  /// \brief Returns the n-th currently active data directory.
  static ezDataDirectoryType* GetDataDirectory(ezUInt32 uiDataDirIndex);

public:

  /// \brief Deletes the given file from all data directories, if possible.
  ///
  /// Files in read-only data directories will not be deleted.
  /// You can use category specifiers to only delete files from certain data directories (see AddDataDirectory).
  /// E.g. "<SETTINGS>MySettings.txt" would only delete "MySettings.txt" from data directories of the "SETTINGS" category.
  static void DeleteFile(const char* szFile);

  /// \brief Checks whether the given file exists in any data directory.
  ///
  /// The search can be restricted to directories of certain categories (see AddDataDirectory).
  static bool ExistsFile(const char* szFile);

  /// \brief Tries to resolve the given path and returns the absolute and relative path to the final file.
  ///
  /// If bForWriting is false, all data directories will be searched for an existing file.
  /// This is similar to what opening a file for reading does. So if there is any file that could be opened for reading,
  /// the path to that file will be returned.
  /// If bForWriting is true, the first valid location where the file could be written to will be returned.
  /// Depending on how data directories are mounted (with read/write access), and which files are already existing,
  /// the file for write output might end up in a place where it has higher or lower open-for-read priority than an identically
  /// named file in another data directory.
  /// out_sAbsolutePath will contain the absolute path to the file. Might be nullptr.
  /// out_sDataDirRelativePath will contain the relative path to the file (from the data directory in which it might end up in). Might be nullptr.
  /// szPath can be an absolute path. This can also be used to find the relative location to the data directory that would handle it.
  /// The function will return EZ_FAILURE if it was not able to determine any location where the file could be read from or written to.
  static ezResult ResolvePath(const char* szPath, bool bForWriting, ezString* out_sAbsolutePath, ezString* out_sDataDirRelativePath);


private:
  friend class ezDataDirectoryReaderWriterBase;
  friend class ezFileReaderBase;
  friend class ezFileWriterBase;

  /// \brief This is used by the actual file readers (like ezFileReader) to get an abstract file reader.
  ///
  /// It tries all data directories, to find the given file.
  /// szFile can be an absolute or relative path.
  /// If bAllowFileEvents is true, the file system will broadcast events about its activity.
  /// This should usually be set to true, unless code is already acting on a file event and needs to do a file operation
  /// itself, which should not trigger an endless recursion of file events.
  static ezDataDirectoryReader* GetFileReader(const char* szFile, bool bAllowFileEvents);

  /// \brief This is used by the actual file writers (like ezFileWriter) to get an abstract file writer.
  ///
  /// It tries all data directories, to find where the given file could be written to.
  /// szFile can be an absolute or relative path.
  /// If bAllowFileEvents is true, the file system will broadcast events about its activity.
  /// This should usually be set to true, unless code is already acting on a file event and needs to do a file operation
  /// itself, which should not trigger an endless recursion of file events.
  static ezDataDirectoryWriter* GetFileWriter(const char* szFile, bool bAllowFileEvents);


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FileSystem);

  /// \brief Called by the Startup System to initialize the file system
  static void Startup();

  /// \brief Called by the Startup System to shutdown the file system
  static void Shutdown();

private:
  /// \brief Returns a list of data directory categories that were embedded in the path.
  static const char* ExtractDataDirsToSearch(const char* szPath, ezHybridArray<ezString, 4>& SearchDirs);

  /// \brief Returns the given path relative to its data directory. The path must be inside the given data directory.
  static const char* GetDataDirRelativePath(const char* szPath, ezUInt32 uiDataDir);

  struct DataDirectory
  {
    DataDirUsage m_Usage;

    ezString m_sCategory;
    ezString m_sGroup;
    ezDataDirectoryType* m_pDataDirectory;
  };

  struct FileSystemData
  {
    ezHybridArray<ezDataDirFactory, 4> m_DataDirFactories;
    ezHybridArray<DataDirectory, 16> m_DataDirectories;
    ezEvent<const FileEvent&> m_Event;
    ezMutex m_Mutex;
  };

  static FileSystemData* s_Data;
};

/// \brief Describes the type of events that are broadcast by the ezFileSystem.
struct ezFileSystem::FileEventType
{
  enum Enum
  {
    None,                       ///< None. Should not occur.
    OpenFileAttempt,            ///< A file is about to be opened for reading.
    OpenFileSucceeded,          ///< A file has been successfully opened for reading.
    OpenFileFailed,             ///< Opening a file for reading failed. Probably because it doesn't exist.
    CreateFileAttempt,          ///< A file is about to be opened for writing.
    CreateFileSucceeded,        ///< A file has been successfully opened for writing.
    CreateFileFailed,           ///< Opening a file for writing failed.
    CloseFile,                  ///< A file was closed.
    AddDataDirectorySucceeded,  ///< A data directory was successfully added.
    AddDataDirectoryFailed,     ///< Adding a data directory failed. No factory could handle it (or there were none).
    RemoveDataDirectory,        ///< A data directory was removed. IMPORTANT: This is where ResourceManagers should check if some loaded resources need to be purged.
    DeleteFile                  ///< A file is about to be deleted.
  };
};

/// \brief The event data that is broadcast by the ezFileSystem upon certain file operations.
struct ezFileSystem::FileEvent
{
  FileEvent();

  /// \brief The exact event that occurred.
  ezFileSystem::FileEventType::Enum m_EventType;

  /// \brief Path to the file or directory that was involved.
  const char* m_szFileOrDirectory;

  /// \brief Additional Path / Name that might be of interest.
  const char* m_szOther;

  /// \brief The data-directory, that was involved.
  const ezDataDirectoryType* m_pDataDir;
};

