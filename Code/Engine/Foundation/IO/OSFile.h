#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Timestamp.h>

struct ezOSFileData;

#if EZ_ENABLED(EZ_USE_POSIX_FILE_API)
  #include <Foundation/IO/Implementation/Posix/OSFileDeclarations_posix.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/IO/Implementation/Win/OSFileDeclarations_win.h>
#endif

/// \brief Defines in which mode to open a file.
struct ezFileMode
{
  enum Enum
  {
    None,   ///< None, only used internally.
    Read,   ///< Open file for reading.
    Write,  ///< Open file for writing (already existing data is discarded).
    Append, ///< Open file for appending (writing, but always only at the end, already existing data is preserved).
  };
};

/// \brief For file seek operations this enum defines from which relative position the seek position is described.
struct ezFilePos
{
  enum Enum
  {
    FromStart,    ///< The seek position is relative to the file's beginning
    FromEnd,      ///< The seek position is relative to the file's end
    FromCurrent,  ///< The seek position is relative to the file's current seek position
  };
};

/// \brief Holds the stats for a file.
struct EZ_FOUNDATION_DLL ezFileStats
{
  ezFileStats();

  /// \brief The name of the file or folder that the stats are for. Does not include the path to it.
  ezStringBuilder m_sFileName;

  /// \brief The last modification time as an UTC timestamp since Unix epoch.
  ezTimestamp m_LastModificationTime;

  /// \brief The size of the file in bytes.
  ezUInt64 m_uiFileSize;

  /// \brief Whether the file object is a file or folder.
  bool m_bIsDirectory;
};

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) || defined(EZ_DOCS)

  struct ezFileIterationData;

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
    /// To iterate all files from on folder, use '/Some/Folder/*'
    /// To iterate over all files of a certain type (in one folder) use '/Some/Folder/*.ext'
    /// Only the final path segment can use placeholders, folders in between must be fully named.
    /// If bRecursive is false, the iterator will only iterate over the files in the start folder, and will not recurse into subdirectories.
    /// If bReportFolders is false, only files will be reported, folders will be skipped (though they will be recursed into, if bRecursive is true).
    ///
    /// If EZ_SUCCESS is returned, the iterator points to a valid file, and the functions GetCurrentPath() and GetStats() will return
    /// the information about that file. To advance to the next file, use Next() or SkipFolder().
    /// When no iteration is possible (the directory does not exist or the wildcards are used incorrectly), EZ_FAILURE is returned.
    ezResult StartSearch(const char* szSearchStart, bool bRecursive = true, bool bReportFolders = true); // [tested]

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

  private:

    /// \brief The current path of the folder, in which the iterator currently is.
    ezStringBuilder m_sCurPath;

    /// \brief Whether to do a recursive file search.
    bool m_bRecursive;

    /// \brief Whether to report folders to the user, or to skip over them.
    bool m_bReportFolders;

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

  /// \brief Opens a file for reading or writing. Returns true if the file could be opened successfully.
  ezResult Open(const char* szFile, ezFileMode::Enum OpenMode); // [tested]

  /// \brief Returns true if a file is currently open.
  bool IsOpen() const; // [tested]

  /// \brief Closes the file, if it is currently opened.
  void Close(); // [tested]

  /// \brief Writes the given number of bytes from the buffer into the file. Returns true if all data was successfully written.
  ezResult Write(const void* pBuffer, ezUInt64 uiBytes); // [tested]

  /// \brief Reads up to the given number of bytes from the file. Returns the actual number of bytes that was read.
  ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes); // [tested]

  /// \brief Returns the name of the file that is currently opened. Returns an empty string, if no file is open.
  const char* GetOpenFileName() const { return m_sFileName.GetData(); } // [tested]

  /// \brief Returns the position in the file at which read/write operations will occur.
  ezUInt64 GetFilePosition() const; // [tested]

  /// \brief Sets the position where in the file to read/write next.
  void SetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const; // [tested]

  /// \brief Returns the current total size of the file.
  ezUInt64 GetFileSize() const; // [tested]

  /// \brief This will return the platform specific file data (handles etc.), if you really want to be able to wreak havoc.
  const ezOSFileData& GetFileData() const { return m_FileData; }

  /// \brief Checks whether the given file exists.
  static bool Exists(const char* szFile); // [tested]

  /// \brief Deletes the given file. Returns EZ_SUCCESS, if the file was deleted or did not exist in the first place. Returns EZ_FAILURE
  static ezResult DeleteFile(const char* szFile); // [tested]

  /// \brief Creates the given directory structure (meaning all directories in the path, that do not exist). Returns false, if any directory could not be created.
  static ezResult CreateDirectoryStructure(const char* szDirectory); // [tested]

  /// \brief Copies the source file into the destination file.
  static ezResult CopyFile(const char* szSource, const char* szDestination); // [tested]

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS) || defined(EZ_DOCS)
  /// \brief Gets the stats about the given file or folder. Returns false, if the stats could not be determined.
  static ezResult GetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats); // [tested]

  /// \brief Useful on systems that are not strict about the casing of file names. Determines the correct name of a file.
  static ezResult GetFileCasing(const char* szFileOrFolder, ezStringBuilder& out_sCorrectSpelling); // [tested]
#endif

  /// \brief Returns the path in which the applications binary file is located.
  static const char* GetApplicationDirectory();

public:

  /// \brief Describes the types of events that ezOSFile sends.
  struct EventType
  {
    enum Enum
    {
      None,
      FileOpen,     ///< A file has been (attempted) to open.
      FileClose,    ///< An open file has been closed.
      FileExists,   ///< A check whether a file exists has been done.
      FileDelete,   ///< A file was attempted to be deleted.
      FileRead,     ///< From an open file data was read.
      FileWrite,    ///< Data was written to an open file.
      MakeDir,      ///< A path has been created (recursive directory creation).
      FileCopy,     ///< A file has been copied to another location.
      FileStat,     ///< The stats of a file are queried
      FileCasing,   ///< The exact spelling of a file/path is requested
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
    ezFileMode::Enum m_FileMode;

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
      m_FileMode = ezFileMode::None;
      m_bSuccess = true;
      m_uiBytesAccessed = 0;
    }
  };

  typedef ezEvent<const EventData&, ezMutex> Event;

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler)    { s_FileEvents.AddEventHandler    (handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_FileEvents.RemoveEventHandler (handler); }

private:

  /// \brief Manages all the Event Handlers for the OSFile events.
  static Event s_FileEvents;

  // *** Internal Functions that do the platform specific work ***

  ezResult InternalOpen(const char* szFile, ezFileMode::Enum OpenMode);
  void InternalClose();
  ezResult InternalWrite(const void* pBuffer, ezUInt64 uiBytes);
  ezUInt64 InternalRead(void* pBuffer, ezUInt64 uiBytes);
  ezUInt64 InternalGetFilePosition() const;
  void InternalSetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const;

  static bool InternalExists(const char* szFile);
  static ezResult InternalDeleteFile(const char* szFile);
  static ezResult InternalCreateDirectory(const char* szFile);

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  static ezResult InternalGetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats);
#endif

  // *************************************************************

  /// \brief Stores the mode with which the file was opened.
  ezFileMode::Enum m_FileMode;

  /// \brief Stores the (cleaned up) filename that was used to open the file.
  ezStringBuilder m_sFileName;

  /// \brief Stores the value of s_FileCounter when the ezOSFile is created.
  ezInt32 m_iFileID;

  /// \brief Platform specific data about the open file.
  ezOSFileData m_FileData;

  /// \brief The application binaries' path.
  static ezString64 s_ApplicationPath;

  /// \brief Counts how many different files are touched.225
  static ezAtomicInteger32 s_FileCounter;
};


