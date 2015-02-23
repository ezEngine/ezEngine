#pragma once

#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Containers/DynamicArray.h>

struct ezDirectoryWatcherImpl
{
  void DoRead();

  HANDLE m_directoryHandle;
  HANDLE m_completionPort;
  bool m_watchSubdirs;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  ezStaticArray<ezUInt8, 4096> m_buffer;
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_bDirectoryOpen(false)
  , m_pImpl(EZ_DEFAULT_NEW(ezDirectoryWatcherImpl))
{
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& path, ezBitflags<Watch> whatToWatch)
{
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
    ezStringWChar(path).GetData(),
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
  m_bDirectoryOpen = true;

  return EZ_SUCCESS;
}

void ezDirectoryWatcher::CloseDirectory()
{
  if (m_bDirectoryOpen)
  {
    CancelIo(m_pImpl->m_directoryHandle);
    CloseHandle(m_pImpl->m_completionPort);
    CloseHandle(m_pImpl->m_directoryHandle);
    m_bDirectoryOpen = false;
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
  ReadDirectoryChangesW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_watchSubdirs,
    m_filter, nullptr, &m_overlapped, nullptr);
}

void ezDirectoryWatcher::EnumerateChanges(ezDelegate<void(const char* filename, Action action)> func)
{
  EZ_ASSERT_DEV(m_bDirectoryOpen, "No directory opened!");
  OVERLAPPED* lpOverlapped;
  DWORD numberOfBytes;
  ULONG_PTR completionKey;
  while (GetQueuedCompletionStatus(m_pImpl->m_completionPort, &numberOfBytes, &completionKey, &lpOverlapped, 0) != 0)
  {
    //Copy the buffer
    EZ_ASSERT_DEBUG(numberOfBytes > 0, "GetQueuedCompletionStatus failed");
    ezHybridArray<ezUInt8, 4096> buffer;
    buffer.SetCount(numberOfBytes);
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
        dir.SetCount(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);
        Action action;
        switch (info->Action)
        {
          case FILE_ACTION_ADDED:
            action = Action::Added;
            break;
          case FILE_ACTION_REMOVED:
            action = Action::Removed;
            break;
          case FILE_ACTION_MODIFIED:
            action = Action::Modified;
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            action = Action::RenamedOldName;
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            action = Action::RenamedNewName;
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
}
