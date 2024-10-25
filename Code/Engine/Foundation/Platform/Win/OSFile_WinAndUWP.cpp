#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

// Defined in Timestamp_Win.cpp
ezInt64 FileTimeToEpoch(FILETIME fileTime);

ezResult ezOSFile::InternalGetFileStats(ezStringView sFileOrFolder, ezFileStats& out_Stats)
{
  ezStringBuilder s = sFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  s.Trim(nullptr, "/\\");

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sParentPath.Clear();
    out_Stats.m_sName = s;
    out_Stats.m_LastModificationTime = ezTimestamp::MakeInvalid();
    return EZ_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(ezDosDevicePath(s), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return EZ_FAILURE;

  out_Stats.m_uiFileSize = ezMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = data.cFileName;
  out_Stats.m_LastModificationTime = ezTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), ezSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return EZ_SUCCESS;
}

ezStringView ezOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    ezUInt32 uiRequiredLength = 512;
    ezHybridArray<wchar_t, 1024> tmp;

    while (true)
    {
      tmp.SetCountUninitialized(uiRequiredLength);

      // reset last error code
      SetLastError(ERROR_SUCCESS);

      const ezUInt32 uiLength = GetModuleFileNameW(nullptr, tmp.GetData(), tmp.GetCount() - 1);
      const DWORD error = GetLastError();

      if (error == ERROR_SUCCESS)
      {
        tmp[uiLength] = L'\0';
        break;
      }

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
        uiRequiredLength += 512;
        continue;
      }

      EZ_REPORT_FAILURE("GetModuleFileNameW failed: {0}", ezArgErrorCode(error));
    }

    s_sApplicationPath = ezStringUtf8(tmp.GetData()).GetData();
  }

  return s_sApplicationPath;
}

const ezString ezOSFile::GetCurrentWorkingDirectory()
{
  const ezUInt32 uiRequiredLength = GetCurrentDirectoryW(0, nullptr);

  ezHybridArray<wchar_t, 1024> tmp;
  tmp.SetCountUninitialized(uiRequiredLength + 16);

  if (GetCurrentDirectoryW(tmp.GetCount() - 1, tmp.GetData()) == 0)
  {
    EZ_REPORT_FAILURE("GetCurrentDirectoryW failed: {}", ezArgErrorCode(GetLastError()));
    return ezString();
  }

  tmp[uiRequiredLength] = L'\0';

  ezStringBuilder clean = ezStringUtf8(tmp.GetData()).GetData();
  clean.MakeCleanPath();

  return clean;
}

#endif
