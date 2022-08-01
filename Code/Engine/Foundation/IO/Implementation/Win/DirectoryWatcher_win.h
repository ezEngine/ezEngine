#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/Implementation/Shared/FileSystemMirror.h>
#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/Logging/Log.h>

// Comment in to get verbose output on the function of the directory watcher
#define DEBUG_FILE_WATCHER

#ifdef DEBUG_FILE_WATCHER
#  define DEBUG_LOG(...) ezLog::Debug(__VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

namespace
{
  struct MoveEvent
  {
    ezString path;
    bool isDirectory = false;

    void Clear()
    {
      path.Clear();
    }

    bool IsEmpty()
    {
      return path.IsEmpty();
    }
  };
} // namespace

struct ezDirectoryWatcherImpl
{
  void DoRead();

  HANDLE m_directoryHandle;
  HANDLE m_completionPort;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  ezDynamicArray<ezUInt8> m_buffer;
  ezBitflags<ezDirectoryWatcher::Watch> m_whatToWatch;
  ezUniquePtr<ezFileSystemMirror<bool>> m_mirror; // store the last modification timestamp alongside each file
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_pImpl(EZ_DEFAULT_NEW(ezDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(1024 * 1024);
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& absolutePath, ezBitflags<Watch> whatToWatch)
{
  EZ_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");
  ezStringBuilder sPath(absolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_filter = FILE_NOTIFY_CHANGE_FILE_NAME;
  if (whatToWatch.IsSet(Watch::Writes))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    m_pImpl->m_mirror = EZ_DEFAULT_NEW(ezFileSystemMirror<bool>);
    m_pImpl->m_mirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_directoryHandle = CreateFileW(ezDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (m_pImpl->m_directoryHandle == INVALID_HANDLE_VALUE)
  {
    return EZ_FAILURE;
  }

  m_pImpl->m_completionPort = CreateIoCompletionPort(m_pImpl->m_directoryHandle, nullptr, 0, 1);
  if (m_pImpl->m_completionPort == INVALID_HANDLE_VALUE)
  {
    return EZ_FAILURE;
  }

  m_pImpl->DoRead();
  m_sDirectoryPath = sPath;

  return EZ_SUCCESS;
}

void ezDirectoryWatcher::CloseDirectory()
{
  if (!m_sDirectoryPath.IsEmpty())
  {
    CancelIo(m_pImpl->m_directoryHandle);
    CloseHandle(m_pImpl->m_completionPort);
    CloseHandle(m_pImpl->m_directoryHandle);
    m_sDirectoryPath.Clear();
  }
}

ezDirectoryWatcher::~ezDirectoryWatcher()
{
  CloseDirectory();
  EZ_DEFAULT_DELETE(m_pImpl);
}

void ezDirectoryWatcherImpl::DoRead()
{
  memset(&m_overlapped, 0, sizeof(m_overlapped));
  BOOL success =
    ReadDirectoryChangesExW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(ezDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
  EZ_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
}

void ezDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, ezUInt32 waitUpToMilliseconds)
{
  EZ_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");
  OVERLAPPED* lpOverlapped;
  DWORD numberOfBytes;
  ULONG_PTR completionKey;
  while (GetQueuedCompletionStatus(m_pImpl->m_completionPort, &numberOfBytes, &completionKey, &lpOverlapped, waitUpToMilliseconds) != 0)
  {
    waitUpToMilliseconds = 0; // only wait on the first call to GetQueuedCompletionStatus
    if (numberOfBytes <= 0)
    {
      ezLog::Debug("GetQueuedCompletionStatus with size 0. Should not happen according to msdn.");
      m_pImpl->DoRead();
      continue;
    }
    // Copy the buffer

    ezHybridArray<ezUInt8, 4096> buffer;
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_pImpl->m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    m_pImpl->DoRead();

    const ezBitflags<ezDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_whatToWatch;

    ezFileSystemMirror<bool>* mirror = m_pImpl->m_mirror.Borrow();

    MoveEvent lastMoveFrom;

    // An orpahend move from is when we see a move from, but no move to
    // This means the file was moved outside of our view of the file system
    // tread this as a delete.
    auto processOrphanedMoveFrom = [&](MoveEvent& moveFrom) {
      if (whatToWatch.IsSet(Watch::Deletes))
      {
        if (!moveFrom.isDirectory)
        {
          func(moveFrom.path.GetData(), ezDirectoryWatcherAction::Removed);
          mirror->RemoveFile(moveFrom.path).AssertSuccess();
        }
        else
        {
          ezStringBuilder dirPath;
          mirror->Enumerate(moveFrom.path, [&](const char* path, ezFileSystemMirror<bool>::Type type) {
                  if (type == ezFileSystemMirror<bool>::Type::File)
                  {
                    func(path, ezDirectoryWatcherAction::Removed);
                  }
                })
            .AssertSuccess();
          mirror->RemoveDirectory(moveFrom.path).AssertSuccess();
        }
      }
      moveFrom.Clear();
    };

    // Progress the messages
    auto info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();
    while (true)
    {
      auto directory = ezArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        ezHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded + 1);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);
        dir[bytesNeeded] = '\0';
        ezDirectoryWatcherAction action = ezDirectoryWatcherAction::None;
        bool fireEvent = false;

        ezStringBuilder eventFilePath = m_sDirectoryPath;
        eventFilePath.AppendPath(dir.GetData());
        eventFilePath.MakeCleanPath();

        if ((info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
          switch (info->Action)
          {
            case FILE_ACTION_ADDED:
              DEBUG_LOG("FILE_ACTION_ADDED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action = ezDirectoryWatcherAction::Added;
              fireEvent = whatToWatch.IsSet(ezDirectoryWatcher::Watch::Creates);
              if (mirror)
              {
                bool fileAlreadyExists= false;
                mirror->AddFile(eventFilePath.GetData(), true, &fileAlreadyExists, nullptr).AssertSuccess();
                if(fileAlreadyExists)
                {
                  fireEvent = false;
                }
              }
              break;
            case FILE_ACTION_REMOVED:
              DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action = ezDirectoryWatcherAction::Removed;
              fireEvent = whatToWatch.IsSet(ezDirectoryWatcher::Watch::Deletes);
              if(mirror)
              {
                mirror->RemoveFile(eventFilePath.GetData()).AssertSuccess();
              }
              break;
            case FILE_ACTION_MODIFIED:
            {
              DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action = ezDirectoryWatcherAction::Modified;
              fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Writes);
              bool fileAreadyKnown = false;
              bool addPending = false;
              mirror->AddFile(eventFilePath.GetData(), false, &fileAreadyKnown, &addPending).AssertSuccess();
              if (fileAreadyKnown && addPending)
              {
                fireEvent = false;
              }
            }
            break;
            case FILE_ACTION_RENAMED_OLD_NAME:
              DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action = ezDirectoryWatcherAction::RenamedOldName;
              fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Renames);
              EZ_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
              lastMoveFrom = {eventFilePath, false};
              break;
            case FILE_ACTION_RENAMED_NEW_NAME:
              DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", eventFilePath, info->LastModificationTime.QuadPart);
              action = ezDirectoryWatcherAction::RenamedNewName;
              fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Renames);
              EZ_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.isDirectory, "last move from doesn't match");
              if(mirror)
              {
                mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
                mirror->AddFile(eventFilePath, false, nullptr, nullptr).AssertSuccess();
              }
              break;
          }

          if (fireEvent)
          {
            func(eventFilePath, action);
          }
        }
        else
        {
          switch (info->Action)
          {
            case FILE_ACTION_ADDED:
            {
              DEBUG_LOG("DIR_ACTION_ADDED {}", eventFilePath);
              if (mirror)
              {
                mirror->AddDirectory(eventFilePath).AssertSuccess();
              }

              // Whenever we add a directory we might be "to late" to see changes inside it.
              // So iterate the file system and make sure we track all files / subdirectories
              ezFileSystemIterator subdirIt;

              subdirIt.StartSearch(eventFilePath.GetData(),
                whatToWatch.IsSet(ezDirectoryWatcher::Watch::Subdirectories)
                  ? ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive
                  : ezFileSystemIteratorFlags::ReportFiles);

              ezStringBuilder tmpPath2;
              for (; subdirIt.IsValid(); subdirIt.Next())
              {
                const ezFileStats& stats = subdirIt.GetStats();
                stats.GetFullPath(tmpPath2);
                if (stats.m_bIsDirectory)
                {
                  if (mirror)
                  {
                    mirror->AddDirectory(tmpPath2).AssertSuccess();
                  }
                }
                else
                {
                  bool fileExistsAlready = false;
                  if (mirror)
                  {
                    mirror->AddFile(tmpPath2, false, &fileExistsAlready, nullptr).AssertSuccess();
                  }
                  if (whatToWatch.IsSet(ezDirectoryWatcher::Watch::Creates) && !fileExistsAlready)
                  {
                    func(tmpPath2, ezDirectoryWatcherAction::Added);
                  }
                }
              }
            }
            break;
            case FILE_ACTION_REMOVED:
              DEBUG_LOG("DIR_ACTION_REMOVED {}", eventFilePath);
              mirror->Enumerate(eventFilePath, [&](const ezStringBuilder& path, ezFileSystemMirror<bool>::Type type) {
                      if (type == ezFileSystemMirror<bool>::Type::File)
                      {
                        func(path, ezDirectoryWatcherAction::Removed);
                      }
                    })
                .AssertSuccess();
              mirror->RemoveDirectory(eventFilePath).AssertSuccess();
              break;
            case FILE_ACTION_RENAMED_OLD_NAME:
              DEBUG_LOG("DIR_ACTION_OLD_NAME {}", eventFilePath);
              lastMoveFrom = {eventFilePath, true};
              break;
            case FILE_ACTION_RENAMED_NEW_NAME:
              DEBUG_LOG("DIR_ACTION_NEW_NAME {}", eventFilePath);
              EZ_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");
              break;
            default:
              break;
          }
        }
      }
      if (info->NextEntryOffset == 0)
        break;
      else
        info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
  }

  if (lpOverlapped != nullptr)
  {
    EZ_ASSERT_DEV(false, "GetQueuedCompletionStatus returned false but lpOverlapped is not null");
  }

  EZ_ASSERT_DEV(GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus gave an error");
}
