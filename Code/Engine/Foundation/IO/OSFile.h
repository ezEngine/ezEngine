#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Timestamp.h>

struct ezOSFileData;

#if EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFileDeclarations_posix.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/OSFileDeclarations_win.h>
#endif

/// \brief Defines in which mode to open a file.
struct ezFileOpenMode
{
  enum Enum
  {
    None,   ///< None, only used internally.
    Read,   ///< Open file for reading.
    Write,  ///< Open file for writing (already existing data is discarded).
    Append, ///< Open file for appending (writing, but always only at the end, already existing data is preserved).
  };
};

/// \brief Holds the stats for a file.
struct EZ_FOUNDATION_DLL ezFileStats
{
  ezFileStats();

  /// \brief Stores the concatenated m_sParentPath and m_sName in \a path.
  void GetFullPath(ezStringBuilder& path) const;

  /// \brief Path to the parent folder.
  /// Append m_sName to m_sParentPath to obtain the full path.
  ezStringBuilder m_sParentPath;

  /// \brief The name of the file or folder that the stats are for. Does not include the parent path to it.
  /// Append m_sName to m_sParentPath to obtain the full path.
  ezString m_sName;

  /// \brief The last modification time as an UTC timestamp since Unix epoch.
  ezTimestamp m_LastModificationTime;

  /// \brief The size of the file in bytes.
  ezUInt64 m_uiFileSize = 0;

  /// \brief Whether the file object is a file or folder.
  bool m_bIsDirectory = false;
};

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) || defined(EZ_DOCS)

struct ezFileIterationData;

struct ezFileSystemIteratorFlags
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Recursive = EZ_BIT(0),
    ReportFiles = EZ_BIT(1),
    ReportFolders = EZ_BIT(2),

    ReportFilesRecursive = Recursive | ReportFiles,
    ReportFoldersRecursive = Recursive | ReportFolders,
    ReportFilesAndFoldersRecursive = Recursive | ReportFiles | ReportFolders,

    Default = ReportFilesAndFoldersRecursive,
  };

  struct Bits
  {
    StorageType Recursive : 1;
    StorageType ReportFiles : 1;
    StorageType ReportFolders : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezFileSystemIteratorFlags);

/// \brief An ezFileSystemIterator allows to iterate over all files in a certain directory.
///
/// The search can be recursive, and it can contain wildcards (* and ?) to limit the search to specific file types.
class EZ_FOUNDATION_DLL ezFileSystemIterator
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezFileSystemIterator);

public:
  ezFileSystemIterator();
  ~ezFileSystemIterator();

  /// \brief Starts a search at the given folder. Use * and ? as wildcards.
  ///
  /// To iterate all files from on folder, use '/Some/Folder'
  /// To iterate over all files of a certain type (in one folder) use '/Some/Folder/*.ext'
  /// Only the final path segment can use placeholders, folders in between must be fully named.
  /// If bRecursive is false, the iterator will only iterate over the files in the start folder, and will not recurse into subdirectories.
  /// If bReportFolders is false, only files will be reported, folders will be skipped (though they will be recursed into, if bRecursive is true).
  ///
  /// If EZ_SUCCESS is returned, the iterator points to a valid file, and the functions GetCurrentPath() and GetStats() will return
  /// the information about that file. To advance to the next file, use Next() or SkipFolder().
  /// When no iteration is possible (the directory does not exist or the wild-cards are used incorrectly), EZ_FAILURE is returned.
  ezResult StartSearch(const char* szSearchStart, ezBitflags<ezFileSystemIteratorFlags> flags = ezFileSystemIteratorFlags::Default); // [tested]

  /// \brief Returns the current path in which files are searched. Changes when 'Next' moves in or out of a sub-folder.
  ///
  /// You can use this to get the full path of the current file, by appending this value and the filename from 'GetStats'
  const ezStringBuilder& GetCurrentPath() const { return m_sCurPath; } // [tested]

  /// \brief Returns the file stats of the current object that the iterator points to.
  const ezFileStats& GetStats() const { return m_CurFile; } // [tested]

  /// \brief Advances the iterator to the next file object. Might recurse into sub-folders.
  ///
  /// Returns false, if the search has reached its end.
  ezResult Next(); // [tested]

  /// \brief The same as 'Next' only that the current folder will not be recursed into.
  ///
  /// Returns false, if the search has reached its end.
  ezResult SkipFolder(); // [tested]

  /// \brief Returns true if the iterator currently points to a valid file entry.
  bool IsValid() const;

private:
  ezInt32 InternalNext();

  /// \brief The current path of the folder, in which the iterator currently is.
  ezStringBuilder m_sCurPath;

  ezBitflags<ezFileSystemIteratorFlags> m_Flags;

  /// \brief The stats about the file that the iterator currently points to.
  ezFileStats m_CurFile;

  /// \brief Platform specific data, required by the implementation.
  ezFileIterationData m_Data;
};

#endif

/// \brief This is an abstraction for the most important file operations.
///
/// Instances of ezOSFile can be used for reading and writing files.
/// All paths must be absolute paths, relative paths and current working directories are not supported,
/// since that cannot be guaranteed to work equally on all platforms under all circumstances.
/// A few static functions allow to query the most important data about files, to delete files and create directories.
class EZ_FOUNDATION_DLL ezOSFile
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezOSFile);

public:
  ezOSFile();
  ~ezOSFile();

  /// \brief Opens a file for reading or writing. Returns EZ_SUCCESS if the file could be opened successfully.
  ezResult Open(const char* szFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode = ezFileShareMode::Default); // [tested]

  /// \brief Returns true if a file is currently open.
  bool IsOpen() const; // [tested]

  /// \brief Closes the file, if it is currently opened.
  void Close(); // [tested]

  /// \brief Writes the given number of bytes from the buffer into the file. Returns true if all data was successfully written.
  ezResult Write(const void* pBuffer, ezUInt64 uiBytes); // [tested]

  /// \brief Reads up to the given number of bytes from the file. Returns the actual number of bytes that was read.
  ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes); // [tested]

  /// \brief Reads the entire file content into the given array
  ezUInt64 ReadAll(ezDynamicArray<ezUInt8>& out_FileContent); // [tested]

  /// \brief Returns the name of the file that is currently opened. Returns an empty string, if no file is open.
  const char* GetOpenFileName() const { return m_sFileName.GetData(); } // [tested]

  /// \brief Returns the position in the file at which read/write operations will occur.
  ezUInt64 GetFilePosition() const; // [tested]

  /// \brief Sets the position where in the file to read/write next.
  void SetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const; // [tested]

  /// \brief Returns the current total size of the file.
  ezUInt64 GetFileSize() const; // [tested]

  /// \brief This will return the platform specific file data (handles etc.), if you really want to be able to wreak havoc.
  const ezOSFileData& GetFileData() const { return m_FileData; }

  /// \brief Returns the processes current working directory (CWD).
  ///
  /// The value typically depends on the directory from which the application was launched.
  /// Since this is a process wide global variable, other code can modify it at any time.
  ///
  /// \note ez does not use the CWD for any file resolution. This function is provided to enable
  /// tools to work with relative paths from the command-line, but every application has to implement
  /// such behavior individually.
  static const ezString GetCurrentWorkingDirectory(); // [tested]

  /// \brief If szPath is a relative path, this function prepends GetCurrentWorkingDirectory().
  ///
  /// In either case, MakeCleanPath() is used before the string is returned.
  static const ezString MakePathAbsoluteWithCWD(const char* szPath); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsFile(const char* szFile); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsDirectory(const char* szDirectory); // [tested]

  /// \brief Deletes the given file. Returns EZ_SUCCESS, if the file was deleted or did not exist in the first place. Returns EZ_FAILURE
  static ezResult DeleteFile(const char* szFile); // [tested]

  /// \brief Creates the given directory structure (meaning all directories in the path, that do not exist). Returns false, if any directory could not be created.
  static ezResult CreateDirectoryStructure(const char* szDirectory); // [tested]

  /// \brief Copies the source file into the destination file.
  static ezResult CopyFile(const char* szSource, const char* szDestination); // [tested]

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS) || defined(EZ_DOCS)
  /// \brief Gets the stats about the given file or folder. Returns false, if the stats could not be determined.
  static ezResult GetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats); // [tested]

#  if (EZ_ENABLED(EZ_SUPPORTS_CASE_INSENSITIVE_PATHS) && EZ_ENABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)) || defined(EZ_DOCS)
  /// \brief Useful on systems that are not strict about the casing of file names. Determines the correct name of a file.
  static ezResult GetFileCasing(const char* szFileOrFolder, ezStringBuilder& out_sCorrectSpelling); // [tested]
#  endif

#endif

#if (EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) && EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)) || defined(EZ_DOCS)

  /// \brief Returns the ezFileStats for all files and folders in the given folder
  static void GatherAllItemsInFolder(ezDynamicArray<ezFileStats>& out_ItemList, const char* szFolder, ezBitflags<ezFileSystemIteratorFlags> flags = ezFileSystemIteratorFlags::Default);

  /// \brief Copies \a szSourceFolder to \a szDestinationFolder. Overwrites existing files.
  static ezResult CopyFolder(const char* szSourceFolder, const char* szDestinationFolder);

  /// \brief Deletes all files recursively in \a szFolder.
  ///
  /// \note The current implementation does not remove the (empty) folders themselves.
  static ezResult DeleteFolder(const char* szFolder);

#endif

  /// \brief Returns the path in which the applications binary file is located.
  static const char* GetApplicationDirectory();

  /// \brief Returns the folder into which user data may be safely written.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the '%appdata%' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static ezString GetUserDataFolder(const char* szSubFolder = nullptr);

  /// \brief Returns the folder into which temp data may be written.
  ///
  /// On Windows this is the '%localappdata%/Temp' directory.
  /// On Posix systems this is the '~/.cache' directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static ezString GetTempDataFolder(const char* szSubFolder = nullptr);

public:
  /// \brief Describes the types of events that ezOSFile sends.
  struct EventType
  {
    enum Enum
    {
      None,
      FileOpen,        ///< A file has been (attempted) to open.
      FileClose,       ///< An open file has been closed.
      FileExists,      ///< A check whether a file exists has been done.
      DirectoryExists, ///< A check whether a directory exists has been done.
      FileDelete,      ///< A file was attempted to be deleted.
      FileRead,        ///< From an open file data was read.
      FileWrite,       ///< Data was written to an open file.
      MakeDir,         ///< A path has been created (recursive directory creation).
      FileCopy,        ///< A file has been copied to another location.
      FileStat,        ///< The stats of a file are queried
      FileCasing,      ///< The exact spelling of a file/path is requested
    };
  };

  /// \brief The data that is sent through the event interface.
  struct EventData
  {
    /// \brief The type of information that is sent.
    EventType::Enum m_EventType;

    /// \brief A unique ID for each file access. Reads and writes to the same open file use the same ID. If the same file is opened multiple times, different IDs are used.
    ezInt32 m_iFileID;

    /// \brief The name of the file that was operated upon.
    const char* m_szFile;

    /// \brief If a second file was operated upon (FileCopy), that is the second file name.
    const char* m_szFile2;

    /// \brief Mode that a file has been opened in.
    ezFileOpenMode::Enum m_FileMode;

    /// \brief Whether the operation succeeded (reading, writing, etc.)
    bool m_bSuccess;

    /// \brief How long the operation took.
    ezTime m_Duration;

    /// \brief How many bytes were transfered (reading, writing)
    ezUInt64 m_uiBytesAccessed;

    EventData()
    {
      m_EventType = EventType::None;
      m_iFileID = 0;
      m_szFile = nullptr;
      m_szFile2 = nullptr;
      m_FileMode = ezFileOpenMode::None;
      m_bSuccess = true;
      m_uiBytesAccessed = 0;
    }
  };

  using Event = ezEvent<const EventData &, ezMutex>;

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_FileEvents.AddEventHandler(handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_FileEvents.RemoveEventHandler(handler); }

private:
  /// \brief Manages all the Event Handlers for the OSFile events.
  static Event s_FileEvents;

  // *** Internal Functions that do the platform specific work ***

  ezResult InternalOpen(const char* szFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode);
  void InternalClose();
  ezResult InternalWrite(const void* pBuffer, ezUInt64 uiBytes);
  ezUInt64 InternalRead(void* pBuffer, ezUInt64 uiBytes);
  ezUInt64 InternalGetFilePosition() const;
  void InternalSetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const;

  static bool InternalExistsFile(const char* szFile);
  static bool InternalExistsDirectory(const char* szDirectory);
  static ezResult InternalDeleteFile(const char* szFile);
  static ezResult InternalCreateDirectory(const char* szFile);

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  static ezResult InternalGetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats);
#endif

  // *************************************************************

  /// \brief Stores the mode with which the file was opened.
  ezFileOpenMode::Enum m_FileMode;

  /// [internal] On win32 when a file is already open, and this is true, ezOSFile will wait until the file becomes available
  bool m_bRetryOnSharingViolation = true;

  /// \brief Stores the (cleaned up) filename that was used to open the file.
  ezStringBuilder m_sFileName;

  /// \brief Stores the value of s_FileCounter when the ezOSFile is created.
  ezInt32 m_iFileID;

  /// \brief Platform specific data about the open file.
  ezOSFileData m_FileData;

  /// \brief The application binaries' path.
  static ezString64 s_ApplicationPath;

  /// \brief The path where user data is stored on this OS
  static ezString64 s_UserDataPath;

  /// \brief The path where temp data is stored on this OS
  static ezString64 s_TempDataPath;

  /// \brief Counts how many different files are touched.225
  static ezAtomicInteger32 s_FileCounter;
};
