#pragma once

#include <Foundation/IO/DirectoryWatcher.h>

#if __has_include(<sys/inotify.h>)
#include <sys/inotify.h>
#include <fcntl.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Implementation/Shared/FileSystemMirror.h>

namespace 
{
  bool IsFile(uint32_t mask)
  {
    return (mask & IN_ISDIR) == 0;
  }

  bool IsDirectory(uint32_t mask)
  {
    return mask & IN_ISDIR;
  }

  struct MoveEvent
  {
    ezString path;
    bool isDirectory = false;
    uint32_t cookie = 0; // Two related events have the same cookie

    void Clear()
    {
      path.Clear();
      cookie = 0;
    }

    bool IsEmpty()
    {
      return path.IsEmpty();
    }
  };
}

struct ezDirectoryWatcherImpl
{
  ezHashTable<int, ezString> m_wdToPath;
  ezMap<ezString, int> m_pathToWd;
  ezString m_topLevelPath;

  int m_inotifyFd = -1;
  uint32_t m_inotifyWatchMask = 0;
  ezBitflags<ezDirectoryWatcher::Watch> m_whatToWatch;
  ezDynamicArray<ezUInt8> m_buffer;
  ezUniquePtr<ezFileSystemMirror> m_fileSystemMirror;
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_pImpl(EZ_DEFAULT_NEW(ezDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(4 * 1024);
  m_pImpl->m_inotifyFd = inotify_init();

  if(m_pImpl->m_inotifyFd > 0)
  {
    // Make the file descriptor non blocking
    int flags = fcntl(m_pImpl->m_inotifyFd, F_GETFL, 0);
    if(fcntl(m_pImpl->m_inotifyFd, F_SETFL, flags | O_NONBLOCK) != 0)
    {
      close(m_pImpl->m_inotifyFd);
      m_pImpl->m_inotifyFd = -1;
    }
  }
}

ezDirectoryWatcher::~ezDirectoryWatcher()
{
  const int inotifyFd = m_pImpl->m_inotifyFd;
  if(inotifyFd >= 0)
  {
    CloseDirectory();
    m_pImpl->m_inotifyFd = -1;
    close(inotifyFd);
  }
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& path, ezBitflags<Watch> whatToWatch)
{
  if(m_pImpl->m_inotifyFd < 0)
  {
    return EZ_FAILURE;
  }

  ezStringBuilder folder = path;
  folder.MakeCleanPath();
  folder.TrimWordEnd("/");

  m_pImpl->m_topLevelPath = folder;

  const int inotifyFd = m_pImpl->m_inotifyFd;

  uint32_t watchMask = 0;


  // If we need to watch subdirectories, we always need to be notified if a subdirectory was created or deleted
  if(whatToWatch.IsSet(Watch::Subdirectories))
  {
    watchMask |= IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO;
  }
  if(whatToWatch.IsSet(Watch::Creates))
  {
    watchMask |= IN_CREATE;
  }
  if(whatToWatch.IsSet(Watch::Reads))
  {
    watchMask |= IN_ACCESS;
  }
  if(whatToWatch.IsSet(Watch::Renames))
  {
    watchMask |= IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO;
  }
  if(whatToWatch.IsSet(Watch::Writes))
  {
    watchMask |= IN_CLOSE_WRITE;
  }

  int wd = inotify_add_watch(inotifyFd, folder.GetData(), watchMask);
  if(wd < 0)
  {
    return EZ_FAILURE;
  }

  if(whatToWatch.IsSet(Watch::Deletes) && whatToWatch.IsSet(Watch::Subdirectories))
  {
    // When a sub-folder is moved out of view. We need to trigger delete events for all files inside it.
    // Thus we need to keep a in memory copy of the file system.
    m_pImpl->m_fileSystemMirror = EZ_DEFAULT_NEW(ezFileSystemMirror);
    m_pImpl->m_fileSystemMirror->AddDirectory(folder.GetData()).AssertSuccess();
    m_pImpl->m_fileSystemMirror->Enumerate(m_pImpl->m_topLevelPath, [](const char* path, ezFileSystemMirror::Type type)
    {
      ezLog::Info("{} {}", type == ezFileSystemMirror::Type::Directory ? "dir" : "file", path);
    }).IgnoreResult();
  }

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_inotifyWatchMask = watchMask;

  ezLog::Info("Now watching {}", folder);
  m_pImpl->m_wdToPath.Insert(wd, folder);
  m_pImpl->m_pathToWd.Insert(folder, wd);

  ezFileSystemIterator dirIt;
  dirIt.StartSearch(folder.GetData(), ezFileSystemIteratorFlags::ReportFoldersRecursive);
  ezStringBuilder subFolderPath;
  while(dirIt.IsValid())
  {
    dirIt.GetStats().GetFullPath(subFolderPath);
    wd = inotify_add_watch(inotifyFd, subFolderPath.GetData(), watchMask);
    if(wd >= 0)
    {
      ezLog::Info("Now watching {}", subFolderPath);
      m_pImpl->m_wdToPath.Insert(wd, subFolderPath);
      m_pImpl->m_pathToWd.Insert(subFolderPath, wd);
    }
    dirIt.Next();
  }

  for(auto it = m_pImpl->m_pathToWd.GetIterator(); it.IsValid(); it.Next())
  {
    ezLog::Info("{} => {}", it.Key(), it.Value());
  }
  

  return EZ_SUCCESS;
}

void ezDirectoryWatcher::CloseDirectory()
{
  const int inotifyFd = m_pImpl->m_inotifyFd;
  if(inotifyFd >= 0)
  {
    for(auto& paths : m_pImpl->m_wdToPath)
    {
      inotify_rm_watch(inotifyFd, paths.Key());
    }
  }
  m_pImpl->m_wdToPath.Clear();
  m_pImpl->m_pathToWd.Clear();
  m_pImpl->m_fileSystemMirror = nullptr;
}


void ezDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func)
{
  const int inotifyFd = m_pImpl->m_inotifyFd;
  uint8_t* const buffer = m_pImpl->m_buffer.GetData();
  const size_t bufferSize = m_pImpl->m_buffer.GetCount();

  ezStringBuilder tmpPath;

  static int eventId = 0;
  const ezBitflags<Watch> whatToWatch = m_pImpl->m_whatToWatch;
  ezFileSystemMirror* mirror = m_pImpl->m_fileSystemMirror.Borrow();

  MoveEvent lastMoveFrom;

  // An orpahend move from is when we see a move from, but no move to
  // This means the file was moved outside of our view of the file system
  // tread this as a delete.
  auto processOrphanedMoveFrom = [&](MoveEvent& moveFrom)
  {
    if(whatToWatch.IsSet(Watch::Renames) && !moveFrom.isDirectory)
    {
      func(moveFrom.path.GetData(), ezDirectoryWatcherAction::Removed);
    }
    moveFrom.Clear();
  };

  if(inotifyFd >= 0)
  {
    ssize_t numBytesRead = 0;
    while((numBytesRead = read(inotifyFd, buffer, bufferSize)) > 0)
    {
      size_t curPos = 0;
      while(curPos < numBytesRead)
      {
        const struct inotify_event* event = (struct inotify_event*)(buffer + curPos);

        auto it = m_pImpl->m_wdToPath.Find(event->wd);
        if(it.IsValid() && event->len > 0)
        {
          tmpPath = it.Value();
          tmpPath.AppendPath(event->name);

          const char* type = "file";
          if(IsDirectory(event->mask))
          {
            type = "folder";
          }

          if(event->mask & IN_CREATE)
          {
            ezLog::Info("IN_CREATE {} {} {}", type, tmpPath, eventId++);

            if(whatToWatch.IsSet(Watch::Subdirectories) && IsDirectory(event->mask))
            {
              int wd = inotify_add_watch(inotifyFd, tmpPath.GetData(), m_pImpl->m_inotifyWatchMask);
              if(wd > 0)
              {
                ezLog::Info("Now watching {}", tmpPath);
                m_pImpl->m_wdToPath.Insert(wd, tmpPath);
                m_pImpl->m_pathToWd.Insert(tmpPath, wd);
              }
            }

            if(mirror != nullptr)
            {
              if(IsFile(event->mask))
              {
                mirror->AddFile(tmpPath).AssertSuccess();
                mirror->Enumerate(m_pImpl->m_topLevelPath, [](const char* path, ezFileSystemMirror::Type type)
                {
                  ezLog::Info("{} {}", type == ezFileSystemMirror::Type::Directory ? "dir" : "file", path);
                }).IgnoreResult();
              }
            }

            if(whatToWatch.IsSet(Watch::Creates) && IsFile(event->mask))
            {
              func(tmpPath.GetData(), ezDirectoryWatcherAction::Added);
            }
          }
          else if(event->mask & IN_CLOSE_WRITE)
          {
            ezLog::Info("IN_CLOSE_WRITE {} {} {}", type, tmpPath, eventId++);
            if(whatToWatch.IsSet(Watch::Writes) && IsFile(event->mask))
            {
              func(tmpPath.GetData(), ezDirectoryWatcherAction::Modified);
            }
          }
          else if(event->mask & IN_ACCESS)
          {
            ezLog::Info("IN_ACCESS {} {} {}", type, tmpPath, eventId++);
            if(whatToWatch.IsSet(Watch::Reads) && IsFile(event->mask))
            {
              func(tmpPath.GetData(), ezDirectoryWatcherAction::Modified);
            }
          }
          else if(event->mask & IN_DELETE)
          {
            ezLog::Info("IN_DELETE {} {} {}", type, tmpPath, eventId++);

            if(mirror)
            {
              if(IsFile(event->mask))
              {
                mirror->RemoveFile(tmpPath).AssertSuccess();
              }
              else
              {
                mirror->RemoveDirectory(tmpPath).AssertSuccess();
              }
            }

            if(whatToWatch.IsSet(Watch::Subdirectories) && IsDirectory(event->mask))
            {
              auto deletedDirIt = m_pImpl->m_pathToWd.Find(tmpPath);
              if(deletedDirIt.IsValid())
              {
                int deletedWd = deletedDirIt.Value();
                deletedDirIt = m_pImpl->m_pathToWd.Remove(deletedDirIt);
                inotify_rm_watch(inotifyFd, deletedWd);
                m_pImpl->m_wdToPath.Remove(deletedWd);
                ezLog::Info("No longer watching {}", tmpPath);
              }
            }

            if(whatToWatch.IsSet(Watch::Renames) && IsFile(event->mask))
            {
              func(tmpPath.GetData(), ezDirectoryWatcherAction::Removed);
            }
          }
          else if(event->mask & IN_MOVED_FROM)
          {
            ezLog::Info("IN_MOVED_FROM {} {} {} ({})", type, tmpPath, eventId++, event->cookie);

            if(!lastMoveFrom.IsEmpty())
            {
              processOrphanedMoveFrom(lastMoveFrom);
            }

            lastMoveFrom.path = tmpPath;
            lastMoveFrom.cookie = event->cookie;
            lastMoveFrom.isDirectory = IsDirectory(event->mask);
          }
          else if(event->mask & IN_MOVED_TO)
          {
            ezLog::Info("IN_MOVED_TO {} {} {} ({})", type, tmpPath, eventId++, event->cookie);

            if(!lastMoveFrom.IsEmpty() && lastMoveFrom.cookie != event->cookie)
            {
              // orphaned move from
              processOrphanedMoveFrom(lastMoveFrom);
            }

            if(lastMoveFrom.IsEmpty())
            {
              // Orphaned move to, treat as add
              if(whatToWatch.IsSet(Watch::Creates))
              {
                if(IsFile(event->mask))
                {
                  func(tmpPath.GetData(), ezDirectoryWatcherAction::Added);
                }
                else
                {
                  // entire directory moved in
                  ezFileSystemIterator fileIterator;
                  fileIterator.StartSearch(tmpPath, ezFileSystemIteratorFlags::ReportFilesRecursive);
                  ezStringBuilder filePath;
                  for(; fileIterator.IsValid(); fileIterator.Next())
                  {
                    fileIterator.GetStats().GetFullPath(filePath);
                    func(filePath, ezDirectoryWatcherAction::Added);
                  }
                }
              }

              if(mirror)
              {
                if(IsFile(event->mask))
                {
                  mirror->AddFile(tmpPath).AssertSuccess();
                }
                else
                {
                  mirror->AddDirectory(tmpPath).AssertSuccess();
                }
              }
            }
            else 
            {
              // regular move
              if(whatToWatch.IsSet(Watch::Renames) && IsFile(event->mask))
              {
                func(lastMoveFrom.path.GetData(), ezDirectoryWatcherAction::RenamedOldName);
                func(tmpPath.GetData(), ezDirectoryWatcherAction::RenamedNewName);
                lastMoveFrom.Clear();
              }

              if(mirror)
              {
                if(IsFile(event->mask))
                {
                  mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
                  mirror->AddFile(tmpPath).AssertSuccess();
                }
                else
                {
                  EZ_ASSERT_NOT_IMPLEMENTED;
                }
              }
            }
          }
        }

        curPos += sizeof(struct inotify_event) + event->len;
      }
    }
  }

  if(!lastMoveFrom.IsEmpty())
  {
    processOrphanedMoveFrom(lastMoveFrom);
  }
}

#else // <sys/inotify.h> is missing
struct ezDirectoryWatcherImpl
{
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_pImpl(nullptr)
{
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& path, ezBitflags<Watch> whatToWatch)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

void ezDirectoryWatcher::CloseDirectory(){EZ_ASSERT_NOT_IMPLEMENTED}

ezDirectoryWatcher::~ezDirectoryWatcher()
{
}

void ezDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func)
{
  EZ_ASSERT_NOT_IMPLEMENTED
}
#endif