#include <Foundation/PCH.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Time/Time.h>

ezString64 ezOSFile::s_ApplicationPath;
ezAtomicInteger32 ezOSFile::s_FileCounter;
ezOSFile::Event ezOSFile::s_FileEvents;

ezFileStats::ezFileStats()
{
  m_LastModificationTime.Invalidate();
  m_uiFileSize = 0;
  m_bIsDirectory = false;
}

ezOSFile::ezOSFile()
{
  m_FileMode = ezFileMode::None;
  m_iFileID = s_FileCounter.Increment();
}

ezOSFile::~ezOSFile()
{
  Close();
}

ezResult ezOSFile::Open(const char* szFile, ezFileMode::Enum OpenMode)
{
  m_iFileID = s_FileCounter.Increment();

  EZ_ASSERT_DEV(OpenMode >= ezFileMode::Read && OpenMode <= ezFileMode::Append, "Invalid Mode");
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

    if (OpenMode == ezFileMode::Write || OpenMode == ezFileMode::Append)
      CreateDirectoryStructure(sFolder.GetData());
  }

  if (InternalOpen(m_sFileName.GetData(), OpenMode) == EZ_SUCCESS)
  {
    m_FileMode = OpenMode;
    Res = EZ_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = ezFileMode::None;
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
  return m_FileMode != ezFileMode::None;
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
  m_FileMode = ezFileMode::None;
}

ezResult ezOSFile::Write(const void* pBuffer, ezUInt64 uiBytes)
{
  EZ_ASSERT_DEV((m_FileMode == ezFileMode::Write) || (m_FileMode == ezFileMode::Append), "The file is not opened for writing.");
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
  EZ_ASSERT_DEV(m_FileMode == ezFileMode::Read, "The file is not opened for reading.");
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

ezUInt64 ezOSFile::GetFilePosition() const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void ezOSFile::SetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  EZ_ASSERT_DEV(m_FileMode != ezFileMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, Pos);
}

ezUInt64 ezOSFile::GetFileSize() const
{
  EZ_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

  const ezInt64 iCurPos = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, ezFilePos::FromEnd);

  const ezUInt64 uiCurSize = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, ezFilePos::FromStart);

  return uiCurSize;
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

  EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

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

ezResult ezOSFile::CopyFile(const char* szSource, const char* szDestination)
{
  const ezTime t0 = ezTime::Now();

  ezOSFile SrcFile, DstFile;

  ezResult Res = EZ_FAILURE;

  if (SrcFile.Open(szSource, ezFileMode::Read) == EZ_FAILURE)
    goto done;

  if (DstFile.Open(szDestination, ezFileMode::Write) == EZ_FAILURE)
    goto done;

  {
    const ezUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

    // can't allocate that much data on the stack
    ezDynamicArray<ezUInt8> TempBuffer;
    TempBuffer.SetCount(uiTempSize);

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
    /// \todo We should implement this also on ezFileSystem, to be able to support stats through virtual filesystems

    const ezTime t0 = ezTime::Now();

    ezStringBuilder s = szFileOrFolder;
    s.MakeCleanPath();
    s.MakePathSeparatorsNative();

    EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

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

  ezResult ezOSFile::GetFileCasing(const char* szFileOrFolder, ezStringBuilder& out_sCorrectSpelling)
  {
    /// \todo We should implement this also on ezFileSystem, to be able to support stats through virtual filesystems

    const ezTime t0 = ezTime::Now();

    ezStringBuilder s(szFileOrFolder);
    s.MakeCleanPath();
    s.MakePathSeparatorsNative();

    EZ_ASSERT_DEV(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

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

      ezFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == EZ_FAILURE)
      {
        Res = EZ_FAILURE;
        break;
      }

      out_sCorrectSpelling.AppendPath(stats.m_sFileName.GetData());

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

#endif // EZ_SUPPORTS_FILE_STATS


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/IO/Implementation/Win/OSFile_win.h>
#elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
  #include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#else
  #error "Unknown Platform."
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OSFile);

