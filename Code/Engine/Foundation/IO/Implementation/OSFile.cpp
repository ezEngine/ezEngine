#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

ezString64 ezOSFile::s_ApplicationPath;
ezString64 ezOSFile::s_UserDataPath;
ezString64 ezOSFile::s_TempDataPath;
ezAtomicInteger32 ezOSFile::s_FileCounter;
ezOSFile::Event ezOSFile::s_FileEvents;

ezFileStats::ezFileStats() = default;

void ezFileStats::GetFullPath(ezStringBuilder& path) const
{
  path.Set(m_sParentPath, "/", m_sName);
  path.MakeCleanPath();
}

ezOSFile::ezOSFile()
{
  m_FileMode = ezFileOpenMode::None;
  m_iFileID = s_FileCounter.Increment();
}

ezOSFile::~ezOSFile()
{
  Close();
}

ezResult ezOSFile::Open(const char* szFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode)
{
  m_iFileID = s_FileCounter.Increment();

  EZ_ASSERT_DEV(OpenMode >= ezFileOpenMode::Read && OpenMode <= ezFileOpenMode::Append, "Invalid Mode");
  EZ_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

  const ezTime t0 = ezTime::Now();

  m_sFileName = szFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  ezResult Res = EZ_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto done;

  {
    ezStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (OpenMode == ezFileOpenMode::Write || OpenMode == ezFileOpenMode::Append)
    {
      EZ_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), OpenMode, FileShareMode) == EZ_SUCCESS)
  {
    m_FileMode = OpenMode;
    Res = EZ_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = ezFileOpenMode::None;
  goto done;

done:
  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_FileMode = OpenMode;
  e.m_iFileID = m_iFileID;
  e.m_szFile = m_sFileName.GetData();
  e.m_EventType = EventType::FileOpen;

  s_FileEvents.Broadcast(e);

  return Res;
}

bool ezOSFile::IsOpen() const
{
  return m_FileMode != ezFileOpenMode::None;
}

void ezOSFile::Close()
{
  if (!IsOpen())
    return;

  const ezTime t0 = ezTime::Now();

  InternalClose();

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = true;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_szFile = m_sFileName.GetData();
  e.m_EventType = EventType::FileClose;

  s_FileEvents.Broadcast(e);

  m_sFileName.Clear();
  m_FileMode = ezFileOpenMode::None;
}

ezResult ezOSFile::Write(const void* pBuffer, ezUInt64 uiBytes)
{
  EZ_ASSERT_DEV((m_FileMode == ezFileOpenMode::Write) || (m_FileMode == ezFileOpenMode::Append), "The file is not opened for writing.");
  EZ_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const ezTime t0 = ezTime::Now();

  const ezResult Res = InternalWrite(pBuffer, uiBytes);

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_szFile = m_sFileName.GetData();
  e.m_EventType = EventType::FileWrite;
  e.m_uiBytesAccessed = uiBytes;

  s_FileEvents.Broadcast(e);

  return Res;
}

ezUInt64 ezOSFile::Read(void* pBuffer, ezUInt64 uiBytes)
{
  EZ_ASSERT_DEV(m_FileMode == ezFileOpenMode::Read, "The file is not opened for reading.");
  EZ_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const ezTime t0 = ezTime::Now();

  const ezUInt64 Res = InternalRead(pBuffer, uiBytes);

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = (Res == uiBytes);
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_szFile = m_sFileName.GetData();
  e.m_EventType = EventType::FileRead;
  e.m_uiBytesAccessed = Res;

  s_FileEvents.Broadcast(e);

  return Res;
}

ezUInt64 ezOSFile::ReadAll(ezDynamicArray<ezUInt8>& out_FileContent)
{
  EZ_ASSERT_DEV(m_FileMode == ezFileOpenMode::Read, "The file is not opened for reading.");

  out_FileContent.Clear();
  out_FileContent.SetCountUninitialized((ezUInt32)GetFileSize());

  if (!out_FileContent.IsEmpty())
  {
    Read(out_FileContent.GetData(), out_FileContent.GetCount());
  }

  return out_FileContent.GetCount();
}

ezUInt64 ezOSFile::GetFilePosition() const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void ezOSFile::SetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  EZ_ASSERT_DEV(m_FileMode != ezFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, Pos);
}

ezUInt64 ezOSFile::GetFileSize() const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

  const ezInt64 iCurPos = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, ezFileSeekMode::FromEnd);

  const ezUInt64 uiCurSize = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, ezFileSeekMode::FromStart);

  return uiCurSize;
}

const ezString ezOSFile::MakePathAbsoluteWithCWD(const char* szPath)
{
  ezStringBuilder tmp = szPath;
  tmp.MakeCleanPath();

  if (tmp.IsRelativePath())
  {
    tmp.PrependFormat("{}/", GetCurrentWorkingDirectory());
    tmp.MakeCleanPath();
  }

  return tmp;
}

bool ezOSFile::ExistsFile(const char* szFile)
{
  const ezTime t0 = ezTime::Now();

  ezStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const bool bRes = InternalExistsFile(s);

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = s;
  e.m_EventType = EventType::FileExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

bool ezOSFile::ExistsDirectory(const char* szDirectory)
{
  const ezTime t0 = ezTime::Now();

  ezStringBuilder s(szDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  EZ_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute");

  const bool bRes = InternalExistsDirectory(s);

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = s;
  e.m_EventType = EventType::DirectoryExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

ezResult ezOSFile::DeleteFile(const char* szFile)
{
  const ezTime t0 = ezTime::Now();

  ezStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const ezResult Res = InternalDeleteFile(s.GetData());

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = szFile;
  e.m_EventType = EventType::FileDelete;

  s_FileEvents.Broadcast(e);

  return Res;
}

ezResult ezOSFile::CreateDirectoryStructure(const char* szDirectory)
{
  const ezTime t0 = ezTime::Now();

  ezStringBuilder s(szDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  ezStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  ezResult Res = EZ_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!ezPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == EZ_FAILURE)
    {
      Res = EZ_FAILURE;
      break;
    }
  }

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = szDirectory;
  e.m_EventType = EventType::MakeDir;

  s_FileEvents.Broadcast(e);

  return Res;
}

ezResult ezOSFile::MoveFileOrDirectory(const char* szDirectoryFrom, const char* szDirectoryTo)
{
  ezStringBuilder sFrom(szDirectoryFrom);
  sFrom.MakeCleanPath();
  sFrom.MakePathSeparatorsNative();

  ezStringBuilder sTo(szDirectoryTo);
  sTo.MakeCleanPath();
  sTo.MakePathSeparatorsNative();

  return InternalMoveFileOrDirectory(sFrom, sTo);
}

ezResult ezOSFile::CopyFile(const char* szSource, const char* szDestination)
{
  const ezTime t0 = ezTime::Now();

  ezOSFile SrcFile, DstFile;

  ezResult Res = EZ_FAILURE;

  if (SrcFile.Open(szSource, ezFileOpenMode::Read) == EZ_FAILURE)
    goto done;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(szDestination, ezFileOpenMode::Write) == EZ_FAILURE)
    goto done;

  {
    const ezUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

    // can't allocate that much data on the stack
    ezDynamicArray<ezUInt8> TempBuffer;
    TempBuffer.SetCountUninitialized(uiTempSize);

    while (true)
    {
      const ezUInt64 uiRead = SrcFile.Read(&TempBuffer[0], uiTempSize);

      if (uiRead == 0)
        break;

      if (DstFile.Write(&TempBuffer[0], uiRead) == EZ_FAILURE)
        goto done;
    }
  }

  Res = EZ_SUCCESS;

done:

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = szSource;
  e.m_szFile2 = szDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

ezResult ezOSFile::GetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats)
{
  const ezTime t0 = ezTime::Now();

  ezStringBuilder s = szFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const ezResult Res = InternalGetFileStats(s.GetData(), out_Stats);

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = szFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if EZ_ENABLED(EZ_SUPPORTS_CASE_INSENSITIVE_PATHS) && EZ_ENABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
ezResult ezOSFile::GetFileCasing(const char* szFileOrFolder, ezStringBuilder& out_sCorrectSpelling)
{
  /// \todo We should implement this also on ezFileSystem, to be able to support stats through virtual filesystems

  const ezTime t0 = ezTime::Now();

  ezStringBuilder s(szFileOrFolder);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  ezStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  out_sCorrectSpelling.Clear();

  ezResult Res = EZ_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!ezPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    if (!sCurPath.IsEmpty())
    {
      ezFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == EZ_FAILURE)
      {
        Res = EZ_FAILURE;
        break;
      }

      out_sCorrectSpelling.AppendPath(stats.m_sName);
    }
    sCurPath.Append(it.GetCharacter());
    ++it;
  }

  const ezTime t1 = ezTime::Now();
  const ezTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == EZ_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_FileCounter.Increment();
  e.m_szFile = szFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // EZ_SUPPORTS_CASE_INSENSITIVE_PATHS && EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // EZ_SUPPORTS_FILE_STATS

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) && EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

void ezOSFile::GatherAllItemsInFolder(ezDynamicArray<ezFileStats>& out_ItemList, const char* szFolder, ezBitflags<ezFileSystemIteratorFlags> flags /*= ezFileSystemIteratorFlags::All*/)
{
  out_ItemList.Clear();

  ezFileSystemIterator iterator;
  iterator.StartSearch(szFolder, flags);

  if (!iterator.IsValid())
    return;

  out_ItemList.Reserve(128);

  while (iterator.IsValid())
  {
    out_ItemList.PushBack(iterator.GetStats());

    iterator.Next();
  }
}

ezResult ezOSFile::CopyFolder(const char* szSourceFolder, const char* szDestinationFolder, ezDynamicArray<ezString>* out_FilesCopied /*= nullptr*/)
{
  ezDynamicArray<ezFileStats> items;
  GatherAllItemsInFolder(items, szSourceFolder);

  ezStringBuilder srcPath;
  ezStringBuilder dstPath;
  ezStringBuilder relPath;

  for (const auto& item : items)
  {
    srcPath = item.m_sParentPath;
    srcPath.AppendPath(item.m_sName);

    relPath = srcPath;

    if (relPath.MakeRelativeTo(szSourceFolder).Failed())
      return EZ_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = szDestinationFolder;
    dstPath.AppendPath(relPath);

    if (item.m_bIsDirectory)
    {
      if (ezOSFile::CreateDirectoryStructure(dstPath).Failed())
        return EZ_FAILURE;
    }
    else
    {
      if (ezOSFile::CopyFile(srcPath, dstPath).Failed())
        return EZ_FAILURE;

      if (out_FilesCopied)
      {
        out_FilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::DeleteFolder(const char* szFolder)
{
  ezDynamicArray<ezFileStats> items;
  GatherAllItemsInFolder(items, szFolder);

  ezStringBuilder fullPath;

  for (const auto& item : items)
  {
    if (item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (ezOSFile::DeleteFile(fullPath).Failed())
      return EZ_FAILURE;
  }

  for (ezUInt32 i = items.GetCount(); i > 0; --i)
  {
    const auto& item = items[i - 1];

    if (!item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (ezOSFile::InternalDeleteDirectory(fullPath).Failed())
      return EZ_FAILURE;
  }

  if (ezOSFile::InternalDeleteDirectory(szFolder).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

#endif // EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) && EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

void ezFileSystemIterator::StartMultiFolderSearch(ezArrayPtr<ezString> startFolders, const char* szSearchTerm, ezBitflags<ezFileSystemIteratorFlags> flags /*= ezFileSystemIteratorFlags::Default*/)
{
  if (startFolders.IsEmpty())
    return;

  m_sMultiSearchTerm = szSearchTerm;
  m_Flags = flags;
  m_uiCurrentStartFolder = 0;
  m_StartFolders = startFolders;

  ezStringBuilder search = startFolders[m_uiCurrentStartFolder];
  search.AppendPath(szSearchTerm);

  StartSearch(search, m_Flags);

  if (!IsValid())
  {
    Next();
  }
}

void ezFileSystemIterator::Next()
{
  while (true)
  {
    const ezInt32 res = InternalNext();

    if (res == 1) // success
    {
      return;
    }
    else if (res == 0) // failure
    {
      ++m_uiCurrentStartFolder;

      if (m_uiCurrentStartFolder < m_StartFolders.GetCount())
      {
        ezStringBuilder search = m_StartFolders[m_uiCurrentStartFolder];
        search.AppendPath(m_sMultiSearchTerm);

        StartSearch(search, m_Flags);
      }
      else
      {
        return;
      }

      if (IsValid())
      {
        return;
      }
    }
    else
    {
      // call InternalNext() again
    }
  }
}

void ezFileSystemIterator::SkipFolder()
{
  EZ_ASSERT_DEBUG(m_Flags.IsSet(ezFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  EZ_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(ezFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(ezFileSystemIteratorFlags::Recursive);
}

#endif


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/OSFile_win.h>

// For UWP we're currently using a mix of WinRT functions and posix.
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#    include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#  endif
#elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#else
#  error "Unknown Platform."
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OSFile);
