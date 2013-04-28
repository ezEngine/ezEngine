#include <Foundation/PCH.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/DynamicArray.h>

ezHybridString<64, ezStaticAllocatorWrapper> ezOSFile::s_ApplicationPath;

ezFileStats::ezFileStats()
{
  m_uiLastModificationTime = 0;
  m_uiFileSize = 0;
  m_bIsDirectory = false;
}

ezOSFile::ezOSFile()
{
  m_FileMode = ezFileMode::None;
}

ezOSFile::~ezOSFile()
{
  Close();
}

ezResult ezOSFile::Open(const char* szFile, ezFileMode::Enum OpenMode)
{
  EZ_ASSERT(OpenMode >= ezFileMode::Read && OpenMode <= ezFileMode::Append, "Invalid Mode");
  EZ_ASSERT(!IsOpen(), "The file has already been opened.");

  m_sFileName = szFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathOsSpecific();

  ezStringBuilder sFolder = m_sFileName.GetFileDirectory();

  CreateDirectoryStructure(sFolder.GetData());

  EZ_ASSERT(m_sFileName.IsAbsolutePath(), "ezOSFile can only open files whose location is given with an absolute path. '%s' is not an absolute path.", m_sFileName.GetData());

  if (InternalOpen(m_sFileName.GetData(), OpenMode) == EZ_SUCCESS)
  {
    m_FileMode = OpenMode;
    return EZ_SUCCESS;
  }

  m_sFileName.Clear();
  m_FileMode = ezFileMode::None;
  return EZ_FAILURE;
}

bool ezOSFile::IsOpen() const
{
  return m_FileMode != ezFileMode::None;
}

void ezOSFile::Close()
{
  if (!IsOpen())
    return;

  InternalClose();

  m_sFileName.Clear();
  m_FileMode = ezFileMode::None;
}

ezResult ezOSFile::Write(const void* pBuffer, ezUInt64 uiBytes)
{
  EZ_ASSERT((m_FileMode == ezFileMode::Write) || (m_FileMode == ezFileMode::Append), "The file is not opened for writing.");
  EZ_ASSERT(pBuffer != NULL, "pBuffer must not be NULL.");

  return InternalWrite(pBuffer, uiBytes);
}

ezUInt64 ezOSFile::Read(void* pBuffer, ezUInt64 uiBytes)
{
  EZ_ASSERT(m_FileMode == ezFileMode::Read, "The file is not opened for reading.");
  EZ_ASSERT(pBuffer != NULL, "pBuffer must not be NULL.");

  return InternalRead(pBuffer, uiBytes);
}

ezUInt64 ezOSFile::GetFilePosition() const
{
  EZ_ASSERT(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void ezOSFile::SetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
  EZ_ASSERT(IsOpen(), "The file must be open to tell the file pointer position.");
  EZ_ASSERT(m_FileMode != ezFileMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, Pos);
}

ezUInt64 ezOSFile::GetFileSize() const
{
  EZ_ASSERT(IsOpen(), "The file must be open to tell the file size.");

  const ezInt64 iCurPos = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, ezFilePos::FromEnd);

  const ezUInt64 uiCurSize = static_cast<ezInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, ezFilePos::FromStart);

  return uiCurSize;
}

bool ezOSFile::Exists(const char* szFile)
{
  ezStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathOsSpecific();

  return InternalExists(szFile);
}

ezResult ezOSFile::DeleteFile(const char* szFile)
{
  ezStringBuilder s(szFile);
  s.MakeCleanPath();
  s.MakePathOsSpecific();

  return InternalDeleteFile(s.GetData());
}

ezResult ezOSFile::CreateDirectoryStructure(const char* szDirectory)
{
  ezStringBuilder s(szDirectory);
  s.MakeCleanPath();
  s.MakePathOsSpecific();

  EZ_ASSERT(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

  ezStringBuilder sCurPath;

  ezStringIterator it = s.GetIteratorFront();

  while (!it.IsEmpty())
  {
    while ((it.GetCharacter() != '\0') && (!ezPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == EZ_FAILURE)
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::CopyFile(const char* szSource, const char* szDestination)
{
  ezOSFile SrcFile, DstFile;

  if (SrcFile.Open(szSource, ezFileMode::Read) == EZ_FAILURE)
    return EZ_FAILURE;

  if (DstFile.Open(szDestination, ezFileMode::Write) == EZ_FAILURE)
    return EZ_FAILURE;

  const ezUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

  // can't allocate that much data on the stack
  ezDynamicArray<ezUInt8> TempBuffer;
  TempBuffer.SetCount(uiTempSize);

  while(true)
  {
    const ezUInt64 uiRead = SrcFile.Read(&TempBuffer[0], uiTempSize);

    if (uiRead == 0)
      break;

    if (DstFile.Write(&TempBuffer[0], uiRead) == EZ_FAILURE)
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

  ezResult ezOSFile::GetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats)
  {
    ezStringBuilder s = szFileOrFolder;
    s.MakeCleanPath();
    s.MakePathOsSpecific();

    EZ_ASSERT(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

    return InternalGetFileStats(s.GetData(), out_Stats);
  }

  ezResult ezOSFile::GetFileCasing(const char* szFileOrFolder, ezStringBuilder& out_sCorrectSpelling)
  {
    ezStringBuilder s(szFileOrFolder);
    s.MakeCleanPath();
    s.MakePathOsSpecific();

    EZ_ASSERT(s.IsAbsolutePath(), "The path '%s' is not absolute.", s.GetData());

    ezStringBuilder sCurPath;

    ezStringIterator it = s.GetIteratorFront();

    out_sCorrectSpelling.Clear();

    while (!it.IsEmpty())
    {
      while ((it.GetCharacter() != '\0') && (!ezPathUtils::IsPathSeparator(it.GetCharacter())))
      {
        sCurPath.Append(it.GetCharacter());
        ++it;
      }

      ezFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == EZ_FAILURE)
        return EZ_FAILURE;

      out_sCorrectSpelling.AppendPath(stats.m_sFileName.GetData());

      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    return EZ_SUCCESS;
  }

#endif // EZ_SUPPORTS_FILE_STATS


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/IO/Implementation/Win/OSFile_win.h>
#elif EZ_USE_POSIX_FILE_API
  #include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#else
  #error "Unknown Platform."
#endif
