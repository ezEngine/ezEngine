#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

struct ezDirectoryWatcherImpl
{
  void DoRead();

  HANDLE m_directoryHandle;
  HANDLE m_completionPort;
  bool m_watchSubdirs;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  ezDynamicArray<ezUInt8> m_buffer;
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
  sPath.ReplaceAll("/", "\\");
  sPath.Trim("\\");

  m_pImpl->m_watchSubdirs = whatToWatch.IsSet(Watch::Subdirectories);
  m_pImpl->m_filter = 0;
  if (whatToWatch.IsSet(Watch::Reads))
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
  if (whatToWatch.IsSet(Watch::Writes))
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
  if (whatToWatch.IsSet(Watch::Creates))
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_CREATION;
  if (whatToWatch.IsSet(Watch::Renames))
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;

  m_pImpl->m_directoryHandle = CreateFileW(
      ezStringWChar(sPath).GetData(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      nullptr);
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
  BOOL success = ReadDirectoryChangesW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_watchSubdirs,
                                       m_filter, nullptr, &m_overlapped, nullptr);
  EZ_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
}

void ezDirectoryWatcher::EnumerateChanges(ezDelegate<void(const char* filename, ezDirectoryWatcherAction action)> func)
{
  EZ_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");
  OVERLAPPED* lpOverlapped;
  DWORD numberOfBytes;
  ULONG_PTR completionKey;
  while (GetQueuedCompletionStatus(m_pImpl->m_completionPort, &numberOfBytes, &completionKey, &lpOverlapped, 0) != 0)
  {
    if (numberOfBytes <= 0)
    {
      ezLog::Debug("GetQueuedCompletionStatus with size 0. Should not happen according to msdn.");
      m_pImpl->DoRead();
      continue;
    }
    //Copy the buffer

    ezHybridArray<ezUInt8, 4096> buffer;
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_pImpl->m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    //Reissue the read request
    m_pImpl->DoRead();

    //Progress the messages
    auto info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();
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
        switch (info->Action)
        {
          case FILE_ACTION_ADDED:
            action = ezDirectoryWatcherAction::Added;
            break;
          case FILE_ACTION_REMOVED:
            action = ezDirectoryWatcherAction::Removed;
            break;
          case FILE_ACTION_MODIFIED:
            action = ezDirectoryWatcherAction::Modified;
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            action = ezDirectoryWatcherAction::RenamedOldName;
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            action = ezDirectoryWatcherAction::RenamedNewName;
            break;
        }
        func(dir.GetData(), action);
      }
      if (info->NextEntryOffset == 0)
        break;
      else
        info = (const FILE_NOTIFY_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
  }

  if (lpOverlapped != nullptr)
  {
    EZ_ASSERT_DEV(false, "GetQueuedCompletionStatus returned false but lpOverlapped is not null");
  }

  DWORD dwError = GetLastError();
  EZ_ASSERT_DEV(dwError == WAIT_TIMEOUT, "GetQueuedCompletionStatus gave an error");
}

