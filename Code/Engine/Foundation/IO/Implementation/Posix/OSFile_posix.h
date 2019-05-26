#pragma once

#include <Foundation/FoundationPCH.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Utilities/CommandLineUtils.h>
#include <errno.h>
#include <sys/stat.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <direct.h>
#define EZ_USE_OLD_POSIX_FUNCTIONS EZ_ON
#else
#include <pwd.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#define EZ_USE_OLD_POSIX_FUNCTIONS EZ_OFF
#endif

#if EZ_ENABLED(EZ_PLATFORM_OSX)
#include <CoreFoundation/CoreFoundation.h>
#endif

ezResult ezOSFile::InternalOpen(const char* szFile, ezFileMode::Enum OpenMode)
{
  switch (OpenMode)
  {
    case ezFileMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case ezFileMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case ezFileMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, ezFilePos::FromEnd);

      break;
    default:
      break;
  }

  return m_FileData.m_pFileHandle != nullptr ? EZ_SUCCESS : EZ_FAILURE;
}

void ezOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

ezResult ezOSFile::InternalWrite(const void* pBuffer, ezUInt64 uiBytes)
{
  const ezUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
      return EZ_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
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
    const ezUInt64 uiReadThisTime = fread(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle);
    uiBytesRead += uiReadThisTime;

    if (uiReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    uiBytesRead += fread(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle);
  }

  return uiBytesRead;
}

ezUInt64 ezOSFile::InternalGetFilePosition() const
{
#if EZ_ENABLED(EZ_USE_OLD_POSIX_FUNCTIONS)
  return static_cast<ezUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<ezUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
#if EZ_ENABLED(EZ_USE_OLD_POSIX_FUNCTIONS)
  switch (Pos)
  {
    case ezFilePos::FromStart:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case ezFilePos::FromEnd:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case ezFilePos::FromCurrent:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case ezFilePos::FromStart:
      EZ_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case ezFilePos::FromEnd:
      EZ_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case ezFilePos::FromCurrent:
      EZ_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#endif
}

bool ezOSFile::InternalExistsFile(const char* szFile)
{
  FILE* pFile = fopen(szFile, "r");

  if (pFile == nullptr)
    return false;

  fclose(pFile);
  return true;
}

// this might not be defined on Windows
#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

bool ezOSFile::InternalExistsDirectory(const char* szDirectory)
{
  struct stat sb;
  return (stat(szDirectory, &sb) == 0 && S_ISDIR(sb.st_mode));
}

ezResult ezOSFile::InternalDeleteFile(const char* szFile)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  int iRes = _unlink(szFile);
#else
  int iRes = unlink(szFile);
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezResult ezOSFile::InternalCreateDirectory(const char* szDirectory)
{
  // handle drive letters as always successful
  if (ezStringUtils::GetCharacterCount(szDirectory) <= 1) // '/'
    return EZ_SUCCESS;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  int iRes = _mkdir(szDirectory);
#else
  int iRes = mkdir(szDirectory, 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return EZ_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to ezOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are not accessible.
  if (errno == EACCES && InternalExistsDirectory(szDirectory))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS) && EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
ezResult ezOSFile::InternalGetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(szFileOrFolder, &tempStat);

  if (iRes != 0)
    return EZ_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sParentPath = szFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = ezPathUtils::GetFileNameAndExtension(szFileOrFolder); // no OS support, so just pass it through
  out_Stats.m_LastModificationTime.SetInt64(tempStat.st_mtime, ezSIUnitOfTime::Second);

  return EZ_SUCCESS;
}
#endif

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

const char* ezOSFile::GetApplicationDirectory()
{
  static ezString256 s_Path;

  if (s_Path.IsEmpty())
  {
#if EZ_ENABLED(EZ_PLATFORM_OSX)

    CFBundleRef appBundle = CFBundleGetMainBundle();
    CFURLRef bundleURL = CFBundleCopyBundleURL(appBundle);
    CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);

    if (bundlePath != nullptr)
    {
      CFIndex length = CFStringGetLength(bundlePath);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

      ezArrayPtr<char> temp = EZ_DEFAULT_NEW_ARRAY(char, maxSize);

      if (CFStringGetCString(bundlePath, temp.GetPtr(), maxSize, kCFStringEncodingUTF8))
      {
        s_Path = temp.GetPtr();
      }

      EZ_DEFAULT_DELETE_ARRAY(temp);
    }

    CFRelease(bundlePath);
    CFRelease(bundleURL);
    CFRelease(appBundle);

#else
    char result[PATH_MAX];
    ssize_t length = readlink( "/proc/self/exe", result, PATH_MAX);
    ezStringBuilder path(ezStringView(result, result + length));
    s_Path = path.GetFileDirectory();
#endif
  }

  return s_Path.GetData();
}

ezString ezOSFile::GetUserDataFolder(const char* szSubFolder)
{
  if (s_UserDataPath.IsEmpty())
  {
    s_UserDataPath = getenv("HOME");

    if (s_UserDataPath.IsEmpty())
      s_UserDataPath = getpwuid(getuid())->pw_dir;
  }

  ezStringBuilder s = s_UserDataPath;
  s.AppendPath(szSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetTempDataFolder(const char* szSubFolder)
{
  if (s_TempDataPath.IsEmpty())
  {
    s_TempDataPath = GetUserDataFolder(".cache").GetData();
  }

  ezStringBuilder s = s_TempDataPath;
  s.AppendPath(szSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif // EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

