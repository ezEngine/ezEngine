#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
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

public:
  /// \brief Describes in which mode a data directory is mounted.
  enum DataDirUsage
  {
    ReadOnly,
    AllowWrites,
  };

  /// \name Data Directory Modifications
  ///
  /// All functions that add / remove data directories are not thread safe and require that this is done
  /// on a single thread with no other thread accessing anything in ezFileSystem simultaneously.
  ///@{

  /// \brief This factory creates a data directory type, if it can handle the given data directory. Otherwise it returns nullptr.
  ///
  /// Every time a data directory is supposed to be added, the file system will query its data dir factories, which one
  /// can successfully create an ezDataDirectoryType. In this process the last factory added has the highest priority.
  /// Once a factory is found that was able to create a ezDataDirectoryType, that one is used.
  /// Different factories can be used to mount different types of data directories. But the same directory can also be
  /// mounted in different ways. For example a simple folder could be mounted on the local system, or via a HTTP server
  /// over a network (lets call it a 'FileServer'). Thus depending on which type of factories are registered, the file system
  /// can provide data from very different sources.
  typedef ezDataDirectoryType* (*ezDataDirFactory)(const char* szDataDirectory, const char* szGroup, const char* szRootName, ezFileSystem::DataDirUsage Usage);

  /// \brief This function allows to register another data directory factory, which might be invoked when a new data directory is to be added.
  static void RegisterDataDirectoryFactory(ezDataDirFactory Factory, float fPriority = 0); // [tested]

  /// \brief Will remove all known data directory factories.
  static void ClearAllDataDirectoryFactories() { s_Data->m_DataDirFactories.Clear(); } // [tested]

  /// \brief Adds a data directory. It will try all the registered factories to find a data directory type that can handle the given path.
  ///
  /// If Usage is ReadOnly, writing to the data directory is not allowed. This is independent of whether the data directory type
  /// COULD write anything.
  /// szGroup defines to what 'group' of data directories this data dir belongs. This is only used in calls to RemoveDataDirectoryGroup,
  /// to remove all data directories of the same group.
  /// You could use groups such as 'Base', 'Project', 'Settings', 'Level', 'Temp' to distinguish between different sets of data directories.
  /// You can also specify the exact same string as szDataDirectory for szGroup, and thus uniquely identify the data dir, to be able to remove just that one.
  /// szRootName is optional for read-only data dirs, but mandatory for writable ones.
  /// It has to be unique to clearly identify a file within that data directory. It must be used when writing to a file in this directory.
  /// For instance, if a data dir root name is "mydata", then the path ":mydata/SomeFile.txt" can be used to write to the top level
  /// folder of this data directory. The same can be used for reading exactly that file and ignoring the other data dirs.
  static ezResult AddDataDirectory(const char* szDataDirectory, const char* szGroup = "", const char* szRootName = "", ezFileSystem::DataDirUsage Usage = ReadOnly); // [tested]

  /// \brief Searches for a data directory with the given root name and removes it
  ///
  /// Returns true, if one was found and removed, false if no such data dir existed.
  static bool RemoveDataDirectory(const char* szRootName);

  /// \brief Removes all data directories that belong to the given group. Returns the number of data directories that were removed.
  static ezUInt32 RemoveDataDirectoryGroup(const char* szGroup); // [tested]

  /// \brief Removes all data directories.
  static void ClearAllDataDirectories(); // [tested]

  /// \brief If a data directory with the given root name already exists, it will be returned, nullptr otherwise.
  static ezDataDirectoryType* FindDataDirectoryWithRoot(const char* szRootName);

  /// \brief Returns the number of currently active data directories.
  static ezUInt32 GetNumDataDirectories(); // [tested]

  /// \brief Returns the n-th currently active data directory.
  static ezDataDirectoryType* GetDataDirectory(ezUInt32 uiDataDirIndex); // [tested]

  /// \brief Calls ezDataDirectoryType::ReloadExternalConfigs() on all active data directories.
  static void ReloadAllExternalDataDirectoryConfigs();

  ///@}
  /// \name Special Directories
  ///@{

  /// \brief Searches for a directory to use as the "Sdk" special directory
  ///
  /// It does so by starting at the directory where the application binary is located and then going up until it finds
  /// a folder that contains the given sub-folder.
  /// The sub-folder is usually where the engine loads the most basic data from, so it should exist.
  ///
  /// Upon success SetSdkRootDirectory() is called with the resulting path.
  ///
  /// \note If the Sdk root directory has been set before, this function does nothing!
  /// It will not override a previously set value. If that is desired, call SetSdkRootDirectory("") first.
  static ezResult DetectSdkRootDirectory(const char* szExpectedSubFolder = "Data/Base");

  /// \brief the special directory ">Sdk" is the root folder of the SDK data, it is often used as the main reference
  /// from where other data directories are found. For higher level code (e.g. ezApplication) it is often vital that this is set early at startup.
  ///
  /// \sa DetectSdkRootDirectory()
  static void SetSdkRootDirectory(const char* szSdkDir);

  /// \brief Returns the previously set Sdk root directory.
  ///
  /// \note Asserts that the path is not empty!
  ///
  /// \sa SetSdkRootDirectory
  /// \sa DetectSdkRootDirectory
  static const char* GetSdkRootDirectory();

  /// \brief Special directories are used when mounting data directories as basic references.
  ///
  /// They are indicated with a ">", ie. ">sdk/Test", but using them is only allowed in few places, e.g. in AddDataDirectory().
  /// Special directories are needed to be able to set up other paths relative to them and to be able to use different
  /// ones on different PCs. For instance when using file-serve functionality, the special directories may be different
  /// on the host and client machines, but the paths used to mount data directories can stay the same because of this.
  static void SetSpecialDirectory(const char* szName, const char* szReplacement);

  /// \brief Returns the absolute path to \a szDirectory.
  ///
  /// If the path starts with a known special directory marker (">marker/") it is replaced accordingly.
  /// See SetSpecialDirectory() for setting custom special directories.
  ///
  /// Built-in special directories (always available) are:
  ///
  /// ">sdk/" - Resolves to what GetSdkRootDirectory() returns.
  /// ">user/" - Resolves to what ezOSFile::GetUserDataFolder() returns.
  /// ">temp/" - Resolves to what ezOSFile::GetTempDataFolder() returns.
  /// ">appdir/" - Resolves to what ezOSFile::GetApplicationDirectory() returns.
  ///
  /// Returns EZ_FAILURE if \a szDirectory starts with an unknown special directory.
  static ezResult ResolveSpecialDirectory(const char* szDirectory, ezStringBuilder& out_Path);

  ///@}

  /// \name Misc
  ///@{

  /// \brief Returns the (recursive) mutex that is used internally by the file system which can be used to guard bundled operations on the file system.
  static ezMutex& GetMutex();

  ///@}

  static ezResult CreateDirectoryStructure(const char* szPath);

public:
  /// \brief Deletes the given file from all data directories, if possible.
  ///
  /// The path must be absolute or rooted, to uniquely identify which file to delete.
  /// For example ":appdata/SomeData.txt", assuming a writable data directory has been mounted with the "appdata" root name.
  static void DeleteFile(const char* szFile); // [tested]

  /// \brief Checks whether the given file exists in any data directory.
  ///
  /// The search can be restricted to directories of certain categories (see AddDataDirectory).
  static bool ExistsFile(const char* szFile); // [tested]

  /// \brief Tries to get the ezFileStats for the given file.
  /// Typically should give the same results as ezOSFile::GetFileStats, but some data dir implementations may not support
  /// retrieving all data (e.g. GetFileStats on folders might not always work).
  static ezResult GetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats);

  /// \brief Tries to resolve the given path and returns the absolute and relative path to the final file.
  ///
  /// If the given path is a rooted path, for instance something like ":appdata/UserData.txt", (which is necessary for writing to files),
  /// the path can be converted easily and the file does not need to exist. Only the data directory with the given root name must be mounted.
  ///
  /// If the path is relative, it is attempted to open the specified file, which means it is searched in all available
  /// data directories. The path to the file that is found will be returned.
  ///
  /// \param out_sAbsolutePath will contain the absolute path to the file. Might be nullptr.
  /// \param out_sDataDirRelativePath will contain the relative path to the file (from the data directory in which it might end up in). Might be nullptr.
  /// \param szPath can be a relative, an absolute or a rooted path. This can also be used to find the relative location to the data directory
  /// that would handle it.
  /// \param out_ppDataDir If not null, it will be set to the data directory that would handle this path.
  ///
  /// \returns The function will return EZ_FAILURE if it was not able to determine any location where the file could be read from or written to.
  static ezResult ResolvePath(const char* szPath, ezStringBuilder* out_sAbsolutePath, ezStringBuilder* out_sDataDirRelativePath, ezDataDirectoryType** out_ppDataDir = nullptr); // [tested]

  /// \brief Starts at szStartDirectory and goes up until it finds a folder that contains the given sub folder structure.
  /// Returns EZ_FAILURE if nothing is found. Otherwise \a result is the absolute path to the existing folder that has a given sub-folder.
  static ezResult FindFolderWithSubPath(const char* szStartDirectory, const char* szSubPath, ezStringBuilder& result); // [tested]

  /// \brief Returns true, if any data directory knows how to redirect the given path. Otherwise the original string is returned in out_sRedirection.
  static bool ResolveAssetRedirection(const char* szPathOrAssetGuid, ezStringBuilder& out_sRedirection);

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
  static ezDataDirectoryReader* GetFileReader(const char* szFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents);

  /// \brief This is used by the actual file writers (like ezFileWriter) to get an abstract file writer.
  ///
  /// It tries all data directories, to find where the given file could be written to.
  /// szFile can be an absolute or relative path.
  /// If bAllowFileEvents is true, the file system will broadcast events about its activity.
  /// This should usually be set to true, unless code is already acting on a file event and needs to do a file operation
  /// itself, which should not trigger an endless recursion of file events.
  static ezDataDirectoryWriter* GetFileWriter(const char* szFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents);


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FileSystem);

  /// \brief Called by the Startup System to initialize the file system
  static void Startup();

  /// \brief Called by the Startup System to shutdown the file system
  static void Shutdown();

private:
  struct DataDirectory
  {
    DataDirUsage m_Usage;

    ezString m_sRootName;
    ezString m_sGroup;
    ezDataDirectoryType* m_pDataDirectory;
  };

  struct Factory
  {
    EZ_DECLARE_POD_TYPE();

    float m_fPriority;
    ezDataDirFactory m_Factory;
  };

  struct FileSystemData
  {
    ezHybridArray<Factory, 4> m_DataDirFactories;
    ezHybridArray<DataDirectory, 16> m_DataDirectories;

    ezEvent<const FileEvent&, ezMutex> m_Event;
    ezMutex m_FsMutex;
  };

  /// \brief Returns a list of data directory categories that were embedded in the path.
  static const char* ExtractRootName(const char* szPath, ezString& rootName);

  /// \brief Returns the given path relative to its data directory. The path must be inside the given data directory.
  static const char* GetDataDirRelativePath(const char* szPath, ezUInt32 uiDataDir);

  static DataDirectory* GetDataDirForRoot(const ezString& sRoot);

  static void CleanUpRootName(ezStringBuilder& sRoot);

  static ezString s_sSdkRootDir;
  static ezMap<ezString, ezString> s_SpecialDirectories;
  static FileSystemData* s_Data;
};

/// \brief Describes the type of events that are broadcast by the ezFileSystem.
struct ezFileSystem::FileEventType
{
  enum Enum
  {
    None,                      ///< None. Should not occur.
    OpenFileAttempt,           ///< A file is about to be opened for reading.
    OpenFileSucceeded,         ///< A file has been successfully opened for reading.
    OpenFileFailed,            ///< Opening a file for reading failed. Probably because it doesn't exist.
    CreateFileAttempt,         ///< A file is about to be opened for writing.
    CreateFileSucceeded,       ///< A file has been successfully opened for writing.
    CreateFileFailed,          ///< Opening a file for writing failed.
    CloseFile,                 ///< A file was closed.
    AddDataDirectorySucceeded, ///< A data directory was successfully added.
    AddDataDirectoryFailed,    ///< Adding a data directory failed. No factory could handle it (or there were none).
    RemoveDataDirectory,       ///< A data directory was removed. IMPORTANT: This is where ResourceManagers should check if some loaded resources need to be purged.
    DeleteFile                 ///< A file is about to be deleted.
  };
};

/// \brief The event data that is broadcast by the ezFileSystem upon certain file operations.
struct ezFileSystem::FileEvent
{
  /// \brief The exact event that occurred.
  ezFileSystem::FileEventType::Enum m_EventType = FileEventType::None;

  /// \brief Path to the file or directory that was involved.
  const char* m_szFileOrDirectory = nullptr;

  /// \brief Additional Path / Name that might be of interest.
  const char* m_szOther = nullptr;

  /// \brief The data-directory, that was involved.
  const ezDataDirectoryType* m_pDataDir = nullptr;
};
