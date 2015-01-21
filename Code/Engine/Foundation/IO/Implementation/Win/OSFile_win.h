#pragma once

// Defined in Timestamp_win.h
ezInt64 FileTimeToEpoch(FILETIME fileTime);

static ezUInt64 HighLowToUInt64(ezUInt32 uiHigh32, ezUInt32 uiLow32)
{
  ezUInt64 uiHigh64 = uiHigh32;
  ezUInt64 uiLow64 = uiLow32;

  return (uiHigh64 << 32) | uiLow64;
}

#if EZ_DISABLED(EZ_USE_POSIX_FILE_API)

ezResult ezOSFile::InternalOpen(const char* szFile, ezFileMode::Enum OpenMode)
{
  ezStringWChar s = szFile;

  switch (OpenMode)
  {
  case ezFileMode::Read:
    m_FileData.m_pFileHandle = CreateFileW(s.GetData(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, nullptr); 
    break;
  case ezFileMode::Write:
    m_FileData.m_pFileHandle = CreateFileW(s.GetData(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,  FILE_ATTRIBUTE_NORMAL, nullptr); 
    break;
  case ezFileMode::Append:
    m_FileData.m_pFileHandle = CreateFileW(s.GetData(), FILE_APPEND_DATA, 0, nullptr, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL, nullptr); 

    // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
    if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
      InternalSetFilePosition(0, ezFilePos::FromEnd);

    break;
  }

  return ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? EZ_SUCCESS : EZ_FAILURE;
}

void ezOSFile::InternalClose()
{
  CloseHandle(m_FileData.m_pFileHandle);
  m_FileData.m_pFileHandle = INVALID_HANDLE_VALUE;
}

ezResult ezOSFile::InternalWrite(const void* pBuffer, ezUInt64 uiBytes)
{
  const ezUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBatchBytes))
      return EZ_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffsetConst(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBytes32))
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezUInt64 ezOSFile::InternalRead(void* pBuffer, ezUInt64 uiBytes)
{
  ezUInt64 uiBytesRead = 0;

  const ezUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;

    if (uiBytesReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;
  }

  return uiBytesRead;
}

ezUInt64 ezOSFile::InternalGetFilePosition() const
{
  long int uiHigh32 = 0;
  ezUInt32 uiLow32 = SetFilePointer(m_FileData.m_pFileHandle, 0, &uiHigh32, FILE_CURRENT);

  return HighLowToUInt64(uiHigh32, uiLow32);
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
  case ezFilePos::FromStart:
    EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
    break;
  case ezFilePos::FromEnd:
    EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
    break;
  case ezFilePos::FromCurrent:
    EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
    break;
  }
}

bool ezOSFile::InternalExists(const char* szFile)
{
  ezStringWChar sPath(szFile);
  DWORD dwAttrib = GetFileAttributesW(sPath.GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

#endif // not EZ_USE_POSIX_FILE_API

ezResult ezOSFile::InternalDeleteFile(const char* szFile)
{
  ezStringWChar s = szFile;
  if (DeleteFileW(s.GetData()) == FALSE)
  {
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
      return EZ_SUCCESS;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::InternalCreateDirectory(const char* szDirectory)
{
  // handle drive letters as always successful
  if (ezStringUtils::GetCharacterCount(szDirectory) <= 3) // 'C:\'
    return EZ_SUCCESS;

  ezStringWChar s = szDirectory;
  if (CreateDirectoryW(s.GetData(), nullptr) == FALSE)
  {
    DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS) 
      return EZ_SUCCESS;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::InternalGetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats)
{
  ezStringBuilder s = szFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  while (ezPathUtils::IsPathSeparator(s.GetIteratorBack().GetCharacter()))
    s.Shrink(0, 1);

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sFileName = s;
    out_Stats.m_LastModificationTime.Invalidate();
    return EZ_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(ezStringWChar(s.GetData()).GetData(), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return EZ_FAILURE;

  out_Stats.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sFileName = data.cFileName;
  out_Stats.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return EZ_SUCCESS;
}

ezFileSystemIterator::ezFileSystemIterator()
{
}

ezFileSystemIterator::~ezFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

ezResult ezFileSystemIterator::StartSearch(const char* szSearchStart, bool bRecursive, bool bReportFolders)
{
  EZ_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  ezStringBuilder sSearch = szSearchStart;
  sSearch.MakeCleanPath();

  m_sCurPath = sSearch.GetFileDirectory();

  EZ_ASSERT_DEV(sSearch.IsAbsolutePath(), "The path '%s' is not absolute.", m_sCurPath.GetData());

  m_bRecursive = bRecursive;
  m_bReportFolders = bReportFolders;

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(ezStringWChar(sSearch.GetData()).GetData(), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return EZ_FAILURE;

  m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sFileName = data.cFileName;
  m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  m_Data.m_Handles.PushBack(hSearch);

  if ((m_CurFile.m_sFileName == "..") || (m_CurFile.m_sFileName == "."))
    return Next(); // will search for the next file or folder that is not ".." or "." ; might return false though

  if (m_CurFile.m_bIsDirectory && !m_bReportFolders)
    return Next();

  return EZ_SUCCESS;
}

ezResult ezFileSystemIterator::Next()
{
  if (m_Data.m_Handles.IsEmpty())
    return EZ_FAILURE;

  if (m_bRecursive && m_CurFile.m_bIsDirectory && (m_CurFile.m_sFileName != "..") && (m_CurFile.m_sFileName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sFileName.GetData());

    ezStringBuilder sNewSearch = m_sCurPath;
    sNewSearch.AppendPath("*");

    WIN32_FIND_DATAW data;
    HANDLE hSearch = FindFirstFileW(ezStringWChar(sNewSearch.GetData()).GetData(), &data);

    if ((hSearch != nullptr) && (hSearch != INVALID_HANDLE_VALUE))
    {
      m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
      m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      m_CurFile.m_sFileName = data.cFileName;
      m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sFileName == "..") || (m_CurFile.m_sFileName == "."))
        return Next(); // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory && !m_bReportFolders)
        return Next();

      return EZ_SUCCESS;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  WIN32_FIND_DATAW data;
  if (!FindNextFileW(m_Data.m_Handles.PeekBack(), &data))
  {
    // nothing found in this directory anymore
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return EZ_FAILURE;

    m_sCurPath.PathParentDirectory();

    return Next();
  }

  m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sFileName = data.cFileName;
  m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  if ((m_CurFile.m_sFileName == "..") || (m_CurFile.m_sFileName == "."))
    return Next();

  if (m_CurFile.m_bIsDirectory && !m_bReportFolders)
    return Next();

  return EZ_SUCCESS;
}

ezResult ezFileSystemIterator::SkipFolder()
{
  EZ_ASSERT_DEV(m_bRecursive, "SkipFolder has no meaning when the iterator is not set to be recursive.");
  EZ_ASSERT_DEV(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_bRecursive = false;

  const ezResult bRet = Next();

  m_bRecursive = true;

  return bRet;
}

const char* ezOSFile::GetApplicationDirectory()
{
  if (s_ApplicationPath.IsEmpty())
  {
    wchar_t szFilename[256];
    GetModuleFileNameW(nullptr, szFilename, 256);

    ezStringBuilder sPath = ezStringUtf8(szFilename).GetData();
    s_ApplicationPath = sPath.GetFileDirectory();
  }

  return s_ApplicationPath.GetData();
}


