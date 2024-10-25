#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

#  include <Shlobj.h>

ezResult ezOSFile::InternalOpen(ezStringView sFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode)
{
  const ezTime sleepTime = ezTime::MakeFromMilliseconds(20);
  ezInt32 iRetries = 20;

  if (FileShareMode == ezFileShareMode::Default)
  {
    // when 'default' share mode is requested, use 'share reads' when opening a file for reading
    // and use 'exclusive' when opening a file for writing

    if (OpenMode == ezFileOpenMode::Read)
    {
      FileShareMode = ezFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = ezFileShareMode::Exclusive;
    }
  }

  DWORD dwSharedMode = 0; // exclusive access
  if (FileShareMode == ezFileShareMode::SharedReads)
  {
    dwSharedMode = FILE_SHARE_READ;
  }

  while (iRetries > 0)
  {
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;

    switch (OpenMode)
    {
      case ezFileOpenMode::Read:
        m_FileData.m_pFileHandle = CreateFileW(ezDosDevicePath(sFile), GENERIC_READ, dwSharedMode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case ezFileOpenMode::Write:
        m_FileData.m_pFileHandle = CreateFileW(ezDosDevicePath(sFile), GENERIC_WRITE, dwSharedMode, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case ezFileOpenMode::Append:
        m_FileData.m_pFileHandle = CreateFileW(ezDosDevicePath(sFile), FILE_APPEND_DATA, dwSharedMode, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
        if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
          InternalSetFilePosition(0, ezFileSeekMode::FromEnd);

        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED
    }

    const ezResult res = ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? EZ_SUCCESS : EZ_FAILURE;

    if (res.Failed())
    {
      if (ezOSFile::ExistsDirectory(sFile))
      {
        // trying to 'open' a directory fails with little useful error codes such as 'access denied'
        return EZ_FAILURE;
      }

      error = GetLastError();

      // file does not exist
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return res;
      // badly formed path, happens when two absolute paths are concatenated
      if (error == ERROR_INVALID_NAME)
        return res;

      if (error == ERROR_SHARING_VIOLATION
          // these two situations happen when the ezInspector is connected
          // for some reason, the networking blocks file reading (when run on the same machine)
          // retrying fixes the problem, but can introduce very long stalls
          || error == WSAEWOULDBLOCK || error == ERROR_SUCCESS)
      {
        if (m_bRetryOnSharingViolation)
        {
          --iRetries;
          ezThreadUtils::Sleep(sleepTime);
          continue; // try again
        }
        else
        {
          return res;
        }
      }

      // anything else, print an error (for now)
      ezLog::Error("CreateFile failed with error {0}", ezArgErrorCode(error));
    }

    return res;
  }

  return EZ_FAILURE;
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
    pBuffer = ezMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
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

  return ezMath::MakeUInt64(uiHigh32, uiLow32);
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
    case ezFileSeekMode::FromStart:
      EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
      break;
    case ezFileSeekMode::FromEnd:
      EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
      break;
    case ezFileSeekMode::FromCurrent:
      EZ_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
      break;
  }
}

bool ezOSFile::InternalExistsFile(ezStringView sFile)
{
  const DWORD dwAttrib = GetFileAttributesW(ezDosDevicePath(sFile).GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

bool ezOSFile::InternalExistsDirectory(ezStringView sDirectory)
{
  const DWORD dwAttrib = GetFileAttributesW(ezDosDevicePath(sDirectory));

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

ezResult ezOSFile::InternalDeleteFile(ezStringView sFile)
{
  if (DeleteFileW(ezDosDevicePath(sFile)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return EZ_SUCCESS;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::InternalDeleteDirectory(ezStringView sDirectory)
{
  if (RemoveDirectoryW(ezDosDevicePath(sDirectory)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return EZ_SUCCESS;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::InternalCreateDirectory(ezStringView sDirectory)
{
  // handle drive letters as always successful
  if (ezStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 3) // 'C:\'
    return EZ_SUCCESS;

  if (CreateDirectoryW(ezDosDevicePath(sDirectory), nullptr) == FALSE)
  {
    const DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS)
      return EZ_SUCCESS;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezOSFile::InternalMoveFileOrDirectory(ezStringView sDirectoryFrom, ezStringView sDirectoryTo)
{
  if (MoveFileW(ezDosDevicePath(sDirectoryFrom), ezDosDevicePath(sDirectoryTo)) == 0)
  {
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

ezString ezOSFile::GetUserDataFolder(ezStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDataPath = ezStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  ezStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetTempDataFolder(ezStringView sSubFolder /*= nullptr*/)
{
  ezStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s = ezStringWChar(pPath);
      s.AppendPath("Temp");
      s_sTempDataPath = s;
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetUserDocumentsFolder(ezStringView sSubFolder /*= {}*/)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_PublicDocuments, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDocumentsPath = ezStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  ezStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif
