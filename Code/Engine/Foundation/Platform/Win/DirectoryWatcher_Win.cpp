#include <Foundation/FoundationPCH.h>

#if (EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) && EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER))

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/Containers/DynamicArray.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/Implementation/FileSystemMirror.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

// Comment in to get verbose output on the function of the directory watcher
// #define DEBUG_FILE_WATCHER

#  ifdef DEBUG_FILE_WATCHER
#    define DEBUG_LOG(...) ezLog::Warning(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

namespace
{
  ezCVarBool cvar_ForceNonNTFS("Platform.DirectoryWatcher.ForceNonNTFS", false, ezCVarFlags::Default, "Forces the use of ReadDirectoryChanges instead of ReadDirectoryChangesEx");

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

  struct Change
  {
    ezStringBuilder eventFilePath;
    bool isFile;
    DWORD Action;
    LARGE_INTEGER LastModificationTime;
  };

  using ezFileSystemMirrorType = ezFileSystemMirror<bool>;

  void GetChangesNTFS(ezStringView sDirectoryPath, const ezHybridArray<ezUInt8, 4096>& buffer, ezDynamicArray<Change>& ref_changes)
  {
    ezUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(uiChanges);
    info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = ezArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        ezHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(ezStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = info->LastModificationTime;
        currentChange.isFile = (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
  }

  void GetChangesNonNTFS(ezStringView sDirectoryPath, const ezHybridArray<ezUInt8, 4096>& buffer, ezDynamicArray<Change>& ref_changes)
  {
    ezUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(ref_changes.GetCount() + uiChanges);
    info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = ezArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        ezHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(ezStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = {};
        currentChange.isFile = true; // Pretend it's a file for now.
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_INFORMATION*)(((ezUInt8*)info) + info->NextEntryOffset);
    }
  }

  void PostProcessNonNTFSChanges(ezDynamicArray<Change>& ref_changes, ezFileSystemMirrorType* pMirror)
  {
    ezHybridArray<ezInt32, 4> nextOp;
    // Figure what changes belong to the same object by creating a linked list of changes. This part is tricky as we basically have to handle all the oddities that ezDirectoryWatcher::EnumerateChanges already does again to figure out which operations belong to the same object.
    {
      ezMap<ezStringView, ezUInt32> lastChangeAtPath;
      nextOp.SetCount(ref_changes.GetCount(), -1);

      ezInt32 pendingRemoveOrRename = -1;
      ezInt32 lastMoveFrom = -1;

      for (ezUInt32 i = 0; i < ref_changes.GetCount(); i++)
      {
        const auto& currentChange = ref_changes[i];
        if (pendingRemoveOrRename != -1 && (currentChange.Action == FILE_ACTION_RENAMED_OLD_NAME) && (ref_changes[pendingRemoveOrRename].eventFilePath == currentChange.eventFilePath))
        {
          // This is the bogus removed event because we changed the casing of a file / directory, ignore.
          lastChangeAtPath.Insert(ref_changes[pendingRemoveOrRename].eventFilePath, pendingRemoveOrRename);
          pendingRemoveOrRename = -1;
        }

        if (pendingRemoveOrRename != -1)
        {
          // An actual remove: Stop tracking the change.
          lastChangeAtPath.Remove(ref_changes[pendingRemoveOrRename].eventFilePath);
          pendingRemoveOrRename = -1;
        }

        ezUInt32* uiUniqueItemIndex = nullptr;
        switch (currentChange.Action)
        {
          case FILE_ACTION_ADDED:
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
          case FILE_ACTION_REMOVED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            pendingRemoveOrRename = i;
            break;
          case FILE_ACTION_MODIFIED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            lastMoveFrom = i;
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            EZ_ASSERT_DEBUG(lastMoveFrom != -1, "last move from should be present when encountering FILE_ACTION_RENAMED_NEW_NAME");
            nextOp[lastMoveFrom] = i;
            lastChangeAtPath.Remove(ref_changes[lastMoveFrom].eventFilePath);
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
        }
      }
    }

    // Anything that is chained via the nextOp linked list must be given the same type.
    // Instead of building arrays of arrays, we create a bit field of all changes and then flatten the linked list at the first set bit. While iterating we remove everything we reached via the linked list so on the next call to get the first bit we will find another object that needs processing. As the operations are ordered, the first bit will always point to the very first operation of an object (nextOp can never point to a previous element).
    ezHybridBitfield<128> pendingChanges;
    pendingChanges.SetCount(ref_changes.GetCount(), true);

    // Get start of first object.
    ezHybridArray<Change*, 4> objectChanges;
    auto it = pendingChanges.GetIterator();
    while (it.IsValid())
    {
      // Flatten the changes for one object into a list for easier processing.
      {
        objectChanges.Clear();
        ezUInt32 currentIndex = it.Value();
        objectChanges.PushBack(&ref_changes[currentIndex]);
        pendingChanges.ClearBit(currentIndex);
        while (nextOp[currentIndex] != -1)
        {
          currentIndex = nextOp[currentIndex];
          pendingChanges.ClearBit(currentIndex);
          objectChanges.PushBack(&ref_changes[currentIndex]);
        }
      }

      // Figure out what type the object is. There is no correct way of doing this, which is the reason why ReadDirectoryChangesExW exists. There are however some good heuristics we can use:
      // 1. If the change is on an existing object, we should know it's type from the mirror. This is 100% correct.
      // 2. If object still exists, we can query its stats on disk to determine its type. In rare cases, this can be wrong but you would need to create a case where a file is replaced with a folder of the same name or something.
      // 3. If the object was created and deleted in the same enumeration, we cannot know what type it was, so we take a guess.
      {
        bool isFile = true;
        ezFileSystemMirrorType::Type type;
        if (pMirror->GetType(objectChanges[0]->eventFilePath, type).Succeeded())
        {
          isFile = type == ezFileSystemMirrorType::Type::File;
        }
        else
        {
          bool typeFound = false;
          for (Change* currentChange : objectChanges)
          {
            ezFileStats stats;
            if (ezOSFile::GetFileStats(currentChange->eventFilePath, stats).Succeeded())
            {
              isFile = !stats.m_bIsDirectory;
              typeFound = true;
              break;
            }
          }
          if (!typeFound)
          {
            // No stats and no entry in mirror: It's guessing time!
            isFile = objectChanges[0]->eventFilePath.FindSubString(".") != nullptr;
          }
        }

        // Apply type to all objects in the chain.
        for (Change* currentChange : objectChanges)
        {
          currentChange->isFile = isFile;
        }
      }

      // Find start of next object.
      it = pendingChanges.GetIterator();
    }
  }

} // namespace

struct ezDirectoryWatcherImpl
{
  void DoRead();
  void EnumerateChangesImpl(ezStringView sDirectoryPath, ezTime waitUpTo, const ezDelegate<void(const Change&)>& callback);

  bool m_bNTFS = false;
  HANDLE m_directoryHandle;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  HANDLE m_overlappedEvent;
  ezDynamicArray<ezUInt8> m_buffer;
  ezBitflags<ezDirectoryWatcher::Watch> m_whatToWatch;
  ezUniquePtr<ezFileSystemMirrorType> m_mirror; // store the last modification timestamp alongside each file
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_pImpl(EZ_DEFAULT_NEW(ezDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(1024 * 1024);
}

ezResult ezDirectoryWatcher::OpenDirectory(ezStringView sAbsolutePath, ezBitflags<Watch> whatToWatch)
{
  m_pImpl->m_bNTFS = false;
  {
    // Get drive root:
    ezStringBuilder sTemp = sAbsolutePath;
    sTemp.MakeCleanPath();
    const char* szFirst = sTemp.FindSubString("/");
    EZ_ASSERT_DEV(szFirst != nullptr, "The path '{}' is not absolute", sTemp);
    ezStringView sRoot = sAbsolutePath.GetSubString(0, static_cast<ezUInt32>(szFirst - sTemp.GetData()) + 1);

    WCHAR szFileSystemName[8];
    BOOL res = GetVolumeInformationW(ezStringWChar(sRoot),
      nullptr,
      0,
      nullptr,
      nullptr,
      nullptr,
      szFileSystemName,
      EZ_ARRAY_SIZE(szFileSystemName));
    m_pImpl->m_bNTFS = res == TRUE && ezStringUtf8(szFileSystemName).GetView() == "NTFS" && !cvar_ForceNonNTFS.GetValue();
  }

  EZ_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");
  ezStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_filter = FILE_NOTIFY_CHANGE_FILE_NAME;
  const bool bRequiresMirror = whatToWatch.IsSet(Watch::Writes) || whatToWatch.AreAllSet(Watch::Deletes | Watch::Subdirectories);
  if (bRequiresMirror)
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
  }

  if (!m_pImpl->m_bNTFS || bRequiresMirror)
  {
    m_pImpl->m_mirror = EZ_DEFAULT_NEW(ezFileSystemMirrorType);
    m_pImpl->m_mirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates | Watch::Renames))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_directoryHandle = CreateFileW(ezDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (m_pImpl->m_directoryHandle == INVALID_HANDLE_VALUE)
  {
    return EZ_FAILURE;
  }

  m_pImpl->m_overlappedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (m_pImpl->m_overlappedEvent == INVALID_HANDLE_VALUE)
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
    CloseHandle(m_pImpl->m_overlappedEvent);
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
  ResetEvent(m_overlappedEvent);
  memset(&m_overlapped, 0, sizeof(m_overlapped));
  m_overlapped.hEvent = m_overlappedEvent;

  if (m_bNTFS)
  {
    BOOL success =
      ReadDirectoryChangesExW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(ezDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
    EZ_ASSERT_DEV(success, "ReadDirectoryChangesExW failed.");
    EZ_IGNORE_UNUSED(success);
  }
  else
  {
    BOOL success =
      ReadDirectoryChangesW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(ezDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr);
    EZ_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
    EZ_IGNORE_UNUSED(success);
  }
}

void ezDirectoryWatcherImpl::EnumerateChangesImpl(ezStringView sDirectoryPath, ezTime waitUpTo, const ezDelegate<void(const Change&)>& callback)
{
  ezHybridArray<Change, 6> changes;

  ezHybridArray<ezUInt8, 4096> buffer;
  while (WaitForSingleObject(m_overlappedEvent, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_OBJECT_0)
  {
    waitUpTo = ezTime::MakeZero(); // only wait on the first call to GetQueuedCompletionStatus

    DWORD numberOfBytes = 0;
    GetOverlappedResult(m_directoryHandle, &m_overlapped, &numberOfBytes, FALSE);

    // Copy the buffer
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    DoRead();

    if (numberOfBytes == 0)
    {
      return;
    }

    // We can fire NTFS events right away as they don't need post processing which prevents us from resizing the changes array unnecessarily.
    if (m_bNTFS)
    {
      GetChangesNTFS(sDirectoryPath, buffer, changes);
      for (const Change& change : changes)
      {
        callback(change);
      }
      changes.Clear();
    }
    else
    {
      GetChangesNonNTFS(sDirectoryPath, buffer, changes);
    }
  }

  // Non-NTFS changes need to be collected and processed in one go to be able to reconstruct the type of the change.
  if (!m_bNTFS)
  {
    PostProcessNonNTFSChanges(changes, m_mirror.Borrow());
    for (const Change& change : changes)
    {
      callback(change);
    }
  }
}

void ezDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, ezTime waitUpTo)
{
  ezFileSystemMirrorType* mirror = m_pImpl->m_mirror.Borrow();
  EZ_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");

  MoveEvent pendingRemoveOrRename;
  const ezBitflags<ezDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_whatToWatch;
  // Renaming a file to the same filename with different casing triggers the events REMOVED (old casing) -> RENAMED_OLD_NAME -> _RENAMED_NEW_NAME.
  // Thus, we need to cache every remove event to make sure the very next event is not a rename of the exact same file.
  auto FirePendingRemove = [&]()
  {
    if (!pendingRemoveOrRename.IsEmpty())
    {
      if (pendingRemoveOrRename.isDirectory)
      {
        if (whatToWatch.IsSet(Watch::Deletes))
        {
          if (mirror && whatToWatch.IsSet(Watch::Subdirectories))
          {
            mirror->Enumerate(pendingRemoveOrRename.path, [&](const ezStringBuilder& sPath, ezFileSystemMirrorType::Type type)
                    { func(sPath, ezDirectoryWatcherAction::Removed, (type == ezFileSystemMirrorType::Type::File) ? ezDirectoryWatcherType::File : ezDirectoryWatcherType::Directory); })
              .AssertSuccess();
          }
          func(pendingRemoveOrRename.path, ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory);
        }
        if (mirror)
        {
          mirror->RemoveDirectory(pendingRemoveOrRename.path).AssertSuccess();
        }
      }
      else
      {
        if (mirror)
        {
          mirror->RemoveFile(pendingRemoveOrRename.path).AssertSuccess();
        }
        if (whatToWatch.IsSet(ezDirectoryWatcher::Watch::Deletes))
        {
          func(pendingRemoveOrRename.path, ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File);
        }
      }
      pendingRemoveOrRename.Clear();
    }
  };

  EZ_SCOPE_EXIT(FirePendingRemove());

  MoveEvent lastMoveFrom;

  // Process the messages
  m_pImpl->EnumerateChangesImpl(m_sDirectoryPath, waitUpTo, [&](const Change& info)
    {
      ezDirectoryWatcherAction action = ezDirectoryWatcherAction::None;
      bool fireEvent = false;

      if (!pendingRemoveOrRename.IsEmpty() && info.isFile == !pendingRemoveOrRename.isDirectory && info.Action == FILE_ACTION_RENAMED_OLD_NAME && pendingRemoveOrRename.path == info.eventFilePath)
      {
        // This is the bogus removed event because we changed the casing of a file / directory, ignore.
        pendingRemoveOrRename.Clear();
      }
      FirePendingRemove();

      if (info.isFile)
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
            DEBUG_LOG("FILE_ACTION_ADDED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = ezDirectoryWatcherAction::Added;
            fireEvent = whatToWatch.IsSet(ezDirectoryWatcher::Watch::Creates);
            if (mirror)
            {
              bool fileAlreadyExists = false;
              mirror->AddFile(info.eventFilePath, true, &fileAlreadyExists, nullptr).AssertSuccess();
              if (fileAlreadyExists)
              {
                fireEvent = false;
              }
            }
            break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = ezDirectoryWatcherAction::Removed;
            fireEvent = false;
            pendingRemoveOrRename = {info.eventFilePath, false};
            break;
          case FILE_ACTION_MODIFIED:
          {
            DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = ezDirectoryWatcherAction::Modified;
            fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Writes);
            bool fileAreadyKnown = false;
            bool addPending = false;
            if (mirror)
            {
              mirror->AddFile(info.eventFilePath, false, &fileAreadyKnown, &addPending).AssertSuccess();
            }
            if (fileAreadyKnown && addPending)
            {
              fireEvent = false;
            }
          }
          break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            EZ_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            action = ezDirectoryWatcherAction::RenamedOldName;
            fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Renames);
            EZ_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
            lastMoveFrom = {info.eventFilePath, false};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = ezDirectoryWatcherAction::RenamedNewName;
            fireEvent = whatToWatch.IsAnySet(ezDirectoryWatcher::Watch::Renames);
            EZ_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.isDirectory, "last move from doesn't match");
            if (mirror)
            {
              mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
              mirror->AddFile(info.eventFilePath, false, nullptr, nullptr).AssertSuccess();
            }
            lastMoveFrom.Clear();
            break;
        }

        if (fireEvent)
        {
          func(info.eventFilePath, action, ezDirectoryWatcherType::File);
        }
      }
      else
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
          {
            DEBUG_LOG("DIR_ACTION_ADDED {}", info.eventFilePath);
            bool directoryAlreadyKnown = false;
            if (mirror)
            {
              mirror->AddDirectory(info.eventFilePath, &directoryAlreadyKnown).AssertSuccess();
            }

            if (whatToWatch.IsSet(Watch::Creates) && !directoryAlreadyKnown)
            {
              func(info.eventFilePath, ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory);
            }

            // Whenever we add a directory we might be "to late" to see changes inside it.
            // So iterate the file system and make sure we track all files / subdirectories
            ezFileSystemIterator subdirIt;

            subdirIt.StartSearch(info.eventFilePath.GetData(),
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
                directoryAlreadyKnown = false;
                if (mirror)
                {
                  mirror->AddDirectory(tmpPath2, &directoryAlreadyKnown).AssertSuccess();
                }
                if (whatToWatch.IsSet(ezDirectoryWatcher::Watch::Creates) && !directoryAlreadyKnown)
                {
                  func(tmpPath2, ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory);
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
                  func(tmpPath2, ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File);
                }
              }
            }
          }
          break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("DIR_ACTION_REMOVED {}", info.eventFilePath);
            pendingRemoveOrRename = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("DIR_ACTION_OLD_NAME {}", info.eventFilePath);
            EZ_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            lastMoveFrom = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("DIR_ACTION_NEW_NAME {}", info.eventFilePath);
            EZ_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");
            if (mirror)
            {
              mirror->MoveDirectory(lastMoveFrom.path, info.eventFilePath).AssertSuccess();
            }
            if (whatToWatch.IsSet(Watch::Renames))
            {
              func(lastMoveFrom.path, ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::Directory);
              func(info.eventFilePath, ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::Directory);
            }
            lastMoveFrom.Clear();
            break;
          default:
            break;
        }
      } //
    });
}


void ezDirectoryWatcher::EnumerateChanges(ezArrayPtr<ezDirectoryWatcher*> watchers, EnumerateChangesFunction func, ezTime waitUpTo)
{
  ezHybridArray<HANDLE, 16> events;
  events.SetCount(watchers.GetCount());

  for (ezUInt32 i = 0; i < watchers.GetCount(); ++i)
  {
    events[i] = watchers[i]->m_pImpl->m_overlappedEvent;
  }

  // Wait for any of the watchers to have some data ready
  if (WaitForMultipleObjects(events.GetCount(), events.GetData(), FALSE, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_TIMEOUT)
  {
    return;
  }

  // Iterate all of them to make sure we report all changes up to this point.
  for (ezDirectoryWatcher* watcher : watchers)
  {
    watcher->EnumerateChanges(func);
  }
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Platform_Win_DirectoryWatcher_Win);
