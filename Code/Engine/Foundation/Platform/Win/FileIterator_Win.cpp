#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>

// Defined in Timestamp_Win.cpp
ezInt64 FileTimeToEpoch(FILETIME fileTime);

ezFileSystemIterator::ezFileSystemIterator() = default;

ezFileSystemIterator::~ezFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool ezFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

void ezFileSystemIterator::StartSearch(ezStringView sSearchStart, ezBitflags<ezFileSystemIteratorFlags> flags /*= ezFileSystemIteratorFlags::All*/)
{
  EZ_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchStart;

  ezStringBuilder sSearch = sSearchStart;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // The Windows documentation disallows trailing (back)slashes.
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  EZ_ASSERT_DEV(flags.IsSet(ezFileSystemIteratorFlags::Recursive) == false || bHasWildcard == false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");

  if (!bHasWildcard && ezOSFile::ExistsDirectory(sSearch))
  {
    // when calling FindFirstFileW with a path to a folder (e.g. "C:/test") it will report "test" as the very first item
    // which is typically NOT what one wants, instead you want items INSIDE that folder to be reported
    // this is especially annoying when 'Recursion' is disabled, as "C:/test" would result in "C:/test" being reported
    // but no items inside it
    // therefore, when the start search points to a directory, we append "/*" to force the search inside the folder
    sSearch.Append("/*");
  }

  m_sCurPath = sSearch.GetFileDirectory();

  EZ_ASSERT_DEV(sSearch.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(ezDosDevicePath(sSearch), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return;

  m_CurFile.m_uiFileSize = ezMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath = m_sCurPath;
  m_CurFile.m_sParentPath.TrimRight("/\\"); // remove trailing slashes
  m_CurFile.m_sName = data.cFileName;
  m_CurFile.m_LastModificationTime = ezTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  m_Data.m_Handles.PushBack(hSearch);

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
  {
    Next(); // will search for the next file or folder that is not ".." or "." ; might return false though
    return;
  }

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFolders))
    {
      Next();
      return;
    }
  }
  else
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFiles))
    {
      Next();
      return;
    }
  }
}

ezInt32 ezFileSystemIterator::InternalNext()
{
  constexpr ezInt32 ReturnFailure = 0;
  constexpr ezInt32 ReturnSuccess = 1;
  constexpr ezInt32 ReturnCallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return ReturnFailure;

  if (m_Flags.IsSet(ezFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName);

    ezStringBuilder sNewSearch = m_sCurPath;
    sNewSearch.AppendPath("*");

    WIN32_FIND_DATAW data;
    HANDLE hSearch = FindFirstFileW(ezDosDevicePath(sNewSearch), &data);

    if ((hSearch != nullptr) && (hSearch != INVALID_HANDLE_VALUE))
    {
      m_CurFile.m_uiFileSize = ezMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
      m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      m_CurFile.m_sParentPath = m_sCurPath;
      EZ_ASSERT_DEBUG(!m_CurFile.m_sParentPath.EndsWith("/") && !m_CurFile.m_sParentPath.EndsWith("\\"), "Unexpected path separator.");
      m_CurFile.m_sName = data.cFileName;
      m_CurFile.m_LastModificationTime = ezTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return ReturnCallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFolders))
          return ReturnCallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFiles))
          return ReturnCallInternalNext;
      }

      return ReturnSuccess;
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
      return ReturnFailure;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1); // Remove trailing /
    }

    return ReturnCallInternalNext;
  }

  m_CurFile.m_uiFileSize = ezMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath = m_sCurPath;
  m_CurFile.m_sParentPath.TrimRight("/\\"); // remove trailing slashes
  m_CurFile.m_sName = data.cFileName;
  m_CurFile.m_LastModificationTime = ezTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return ReturnCallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFolders))
      return ReturnCallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFiles))
      return ReturnCallInternalNext;
  }

  return ReturnSuccess;
}

#endif
