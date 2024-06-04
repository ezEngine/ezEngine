#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
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
  static ezEventSubscriptionID RegisterEventHandler(ezEvent<const FileEvent&>::Handler handler);

  /// \brief Unregisters a previously registered Event Handler.
  static void UnregisterEventHandler(ezEvent<const FileEvent&>::Handler handler);
  static void UnregisterEventHandler(ezEventSubscriptionID subscriptionId);

public:
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
  using ezDataDirFactory = ezDataDirectoryType* (*)(ezStringView, ezStringView, ezStringView, ezDataDirUsage);

  /// \brief This function allows to register another data directory factory, which might be invoked when a new data directory is to be added.
  static void RegisterDataDirectoryFactory(ezDataDirFactory factory, float fPriority = 0); // [tested]

  /// \brief Will remove all known data directory factories.
  static void ClearAllDataDirectoryFactories() { s_pData->m_DataDirFactories.Clear(); } // [tested]

  /// \brief Adds a data directory. It will try all the registered factories to find a data directory type that can handle the given path.
  ///
  /// If Usage is ReadOnly, writing to the data directory is not allowed. This is independent of whether the data directory type
  /// COULD write anything.
  /// szGroup defines to what 'group' of data directories this data dir belongs. This is only used in calls to RemoveDataDirectoryGroup,
  /// to remove all data directories of the same group.
  /// You could use groups such as 'Base', 'Project', 'Settings', 'Level', 'Temp' to distinguish between different sets of data directories.
  /// You can also specify the exact same string as szDataDirectory for szGroup, and thus uniquely identify the data dir, to be able to remove just
  /// that one. szRootName is optional for read-only data dirs, but mandatory for writable ones. It has to be unique to clearly identify a file within
  /// that data directory. It must be used when writing to a file in this directory. For instance, if a data dir root name is "mydata", then the path
  /// ":mydata/SomeFile.txt" can be used to write to the top level folder of this data directory. The same can be used for reading exactly that file
  /// and ignoring the other data dirs.
  static ezResult AddDataDirectory(ezStringView sDataDirectory, ezStringView sGroup = {}, ezStringView sRootName = {}, ezDataDirUsage usage = ezDataDirUsage::ReadOnly); // [tested]

  /// \brief Searches for a data directory with the given root name and removes it
  ///
  /// Returns true, if one was found and removed, false if no such data dir existed.
  static bool RemoveDataDirectory(ezStringView sRootName);

  /// \brief Removes all data directories that belong to the given group. Returns the number of data directories that were removed.
  static ezUInt32 RemoveDataDirectoryGroup(ezStringView sGroup); // [tested]

  /// \brief Removes all data directories.
  static void ClearAllDataDirectories(); // [tested]

  /// \brief If a data directory with the given root name already exists, it will be returned, nullptr otherwise.
  static const ezDataDirectoryInfo* FindDataDirectoryWithRoot(ezStringView sRootName);

  /// \brief Returns the number of currently active data directories.
  static ezUInt32 GetNumDataDirectories(); // [tested]

  /// \brief Returns the n-th currently active data directory.
  static ezDataDirectoryType* GetDataDirectory(ezUInt32 uiDataDirIndex); // [tested]

  /// \brief Returns the info about the n-th currently active data directory.
  static const ezDataDirectoryInfo& GetDataDirectoryInfo(ezUInt32 uiDataDirIndex);

  /// \brief Calls ezDataDirectoryType::ReloadExternalConfigs() on all active data directories.
  static void ReloadAllExternalDataDirectoryConfigs();

  ///@}
  /// \name Special Directories
  ///@{

  /// \brief Searches for a directory to use as the "Sdk" special directory
  ///
  /// It does so by starting at the directory where the application binary is located and then goes up
  /// until it finds a folder that contains the given sub-folder. The sub-folder is usually where the
  /// engine loads the most basic data from, so it should exist.
  ///
  /// Additionally the 'redirection file' feature of ezFileSystem::FindFolderWithSubPath() is used
  /// to allow finding a relocated SDK folder. To do that, place a file called 'ezSdkRoot.txt'
  /// in your top level folder. It should contain the relative path pointing to the SDK folder.
  ///
  /// Upon success SetSdkRootDirectory() is called with the resulting path.
  ///
  /// \note If the Sdk root directory has been set before, this function does nothing!
  /// It will not override a previously set value. If that is desired, call SetSdkRootDirectory("") first.
  ///
  /// \sa ezFileSystem::FindFolderWithSubPath()
  static ezResult DetectSdkRootDirectory(ezStringView sExpectedSubFolder = "Data/Base");

  /// \brief the special directory ">Sdk" is the root folder of the SDK data, it is often used as the main reference
  /// from where other data directories are found. For higher level code (e.g. ezApplication) it is often vital that this is set early at startup.
  ///
  /// \sa DetectSdkRootDirectory()
  static void SetSdkRootDirectory(ezStringView sSdkDir);

  /// \brief Returns the previously set Sdk root directory.
  ///
  /// \note Asserts that the path is not empty!
  ///
  /// \sa SetSdkRootDirectory
  /// \sa DetectSdkRootDirectory
  static ezStringView GetSdkRootDirectory();

  /// \brief Special directories are used when mounting data directories as basic references.
  ///
  /// They are indicated with a ">", ie. ">sdk/Test", but using them is only allowed in few places, e.g. in AddDataDirectory().
  /// Special directories are needed to be able to set up other paths relative to them and to be able to use different
  /// ones on different PCs. For instance when using file-serve functionality, the special directories may be different
  /// on the host and client machines, but the paths used to mount data directories can stay the same because of this.
  static void SetSpecialDirectory(ezStringView sName, ezStringView sReplacement);

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
  static ezResult ResolveSpecialDirectory(ezStringView sDirectory, ezStringBuilder& out_sPath);

  ///@}

  /// \name Misc
  ///@{

  /// \brief Returns the (recursive) mutex that is used internally by the file system which can be used to guard bundled operations on the file
  /// system.
  static ezMutex& GetMutex();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  /// \brief Starts a multi-folder search for \a szSearchTerm on all current data directories.
  static void StartSearch(ezFileSystemIterator& ref_iterator, ezStringView sSearchTerm, ezBitflags<ezFileSystemIteratorFlags> flags = ezFileSystemIteratorFlags::Default);
#endif

  ///@}

  static ezResult CreateDirectoryStructure(ezStringView sPath);

public:
  /// \brief Deletes the given file from all data directories, if possible.
  ///
  /// The path must be absolute or rooted, to uniquely identify which file to delete.
  /// For example ":appdata/SomeData.txt", assuming a writable data directory has been mounted with the "appdata" root name.
  static void DeleteFile(ezStringView sFile); // [tested]

  /// \brief Checks whether the given file exists in any data directory.
  ///
  /// The search can be restricted to directories of certain categories (see AddDataDirectory).
  static bool ExistsFile(ezStringView sFile); // [tested]

  /// \brief Tries to get the ezFileStats for the given file.
  /// Typically should give the same results as ezOSFile::GetFileStats, but some data dir implementations may not support
  /// retrieving all data (e.g. GetFileStats on folders might not always work).
  static ezResult GetFileStats(ezStringView sFileOrFolder, ezFileStats& out_stats);

  /// \brief Tries to resolve the given path and returns the absolute and relative path to the final file.
  ///
  /// If the given path is a rooted path, for instance something like ":appdata/UserData.txt", (which is necessary for writing to files),
  /// the path can be converted easily and the file does not need to exist. Only the data directory with the given root name must be mounted.
  ///
  /// If the path is relative, it is attempted to open the specified file, which means it is searched in all available
  /// data directories. The path to the file that is found will be returned.
  ///
  /// \param sPath can be a relative, an absolute or a rooted path. This can also be used to find the relative location to the data
  /// directory that would handle it.
  /// \param out_sAbsolutePath will contain the absolute path to the file. Can be nullptr.
  /// \param out_sDataDirRelativePath will contain the relative path to the file (from the data directory in which it might end up in). Can be
  /// nullptr.
  /// \param out_ppDataDir If not null, it will be set to the data directory that would handle this path.
  ///
  /// \returns The function will return EZ_FAILURE if it was not able to determine any location where the file could be read from or written to.
  static ezResult ResolvePath(ezStringView sPath, ezStringBuilder* out_pAbsolutePath, ezStringBuilder* out_pDataDirRelativePath, const ezDataDirectoryInfo** out_pDataDir = nullptr); // [tested]

  /// \brief Starts at szStartDirectory and goes up until it finds a folder that contains the given sub folder structure.
  ///
  /// Returns EZ_FAILURE if nothing is found. Otherwise \a result is the absolute path to the existing folder that has a given sub-folder.
  ///
  /// \param result If successful, this contains the folder path in which szSubPath exists.
  /// \param szStartDirectory The directory in which to start the search and iterate upwards.
  /// \param szSubPath the relative path to look for in each visited directory. The function succeeds if such a file or folder is found.
  /// \param szRedirectionFileName An optional file name for a redirection file. If in any visited folder a file with this name is found, it will be opened, read entirely, and appended to the current search path, and it is checked whether \a szSubPath can be found there. This step is not recursive and can't result in an endless loop. It allows to relocate the SDK folder and still have it found, by placing such a redirection file. A common use case, is when ezEngine is used as a Git submodule and therefore the overall file structure is slightly different.
  static ezResult FindFolderWithSubPath(ezStringBuilder& ref_sResult, ezStringView sStartDirectory, ezStringView sSubPath, ezStringView sRedirectionFileName = {}); // [tested]

  /// \brief Returns true, if any data directory knows how to redirect the given path. Otherwise the original string is returned in out_sRedirection.
  static bool ResolveAssetRedirection(ezStringView sPathOrAssetGuid, ezStringBuilder& out_sRedirection);

  /// \brief Migrates a file from an old location to a new one, and returns the path that should be used to open it (either the old or the new path).
  ///
  /// If the file does not exist in the old location, nothing is done, and the new location is returned.
  /// Otherwise, it is attempted to move the file from the old location to the new location.
  /// In case that fails (target not writeable or so), the old path is returned, so that code that needs to read that file, finds it in the correct location.
  /// If it succeeds, the new location is returned. Afterwards, the file does not exist in the old location anymore.
  static ezStringView MigrateFileLocation(ezStringView sOldLocation, ezStringView sNewLocation);

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
  static ezDataDirectoryReader* GetFileReader(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents);

  /// \brief This is used by the actual file writers (like ezFileWriter) to get an abstract file writer.
  ///
  /// It tries all data directories, to find where the given file could be written to.
  /// szFile can be an absolute or relative path.
  /// If bAllowFileEvents is true, the file system will broadcast events about its activity.
  /// This should usually be set to true, unless code is already acting on a file event and needs to do a file operation
  /// itself, which should not trigger an endless recursion of file events.
  static ezDataDirectoryWriter* GetFileWriter(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents);


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FileSystem);

  /// \brief Called by the Startup System to initialize the file system
  static void Startup();

  /// \brief Called by the Startup System to shutdown the file system
  static void Shutdown();

private:
  struct Factory
  {
    EZ_DECLARE_POD_TYPE();

    float m_fPriority;
    ezDataDirFactory m_Factory;
  };

  struct FileSystemData
  {
    ezHybridArray<Factory, 4> m_DataDirFactories;
    ezHybridArray<ezDataDirectoryInfo, 16> m_DataDirectories;

    ezEvent<const FileEvent&, ezMutex> m_Event;
    ezMutex m_FsMutex;
  };

  /// \brief Extracts the root name in a rooted path, e.g. for ":bin/stuff" it would extract "bin". Returns the relative path (here "stuff") or an empty string if it is a root only.
  static ezStringView ExtractRootName(ezStringView sFile, ezString& rootName);

  /// \brief Returns the given path relative to its data directory. The path must be inside the given data directory.
  static ezStringView GetDataDirRelativePath(ezStringView sFile, ezUInt32 uiDataDir);

  static ezDataDirectoryInfo* GetDataDirForRoot(const ezString& sRoot);

  static void CleanUpRootName(ezStringBuilder& sRoot);

  static ezString s_sSdkRootDir;
  static ezMap<ezString, ezString> s_SpecialDirectories;
  static FileSystemData* s_pData;
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
    RemoveDataDirectory,       ///< A data directory was removed. IMPORTANT: This is where ResourceManagers should check if some loaded resources need to be
                               ///< purged.
    DeleteFile                 ///< A file is about to be deleted.
  };
};

/// \brief The event data that is broadcast by the ezFileSystem upon certain file operations.
struct ezFileSystem::FileEvent
{
  /// \brief The exact event that occurred.
  ezFileSystem::FileEventType::Enum m_EventType = FileEventType::None;

  /// \brief Path to the file or directory that was involved.
  ezStringView m_sFileOrDirectory;

  /// \brief Additional Path / Name that might be of interest.
  ezStringView m_sOther;

  /// \brief The data-directory, that was involved.
  const ezDataDirectoryType* m_pDataDir = nullptr;
};
