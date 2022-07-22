#pragma once

#include <Foundation/FoundationPCH.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <direct.h>
#  define EZ_USE_OLD_POSIX_FUNCTIONS EZ_ON
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#  define EZ_USE_OLD_POSIX_FUNCTIONS EZ_OFF
#endif

#if EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <CoreFoundation/CoreFoundation.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

ezResult ezOSFile::InternalOpen(const char* szFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode)
{
  switch (OpenMode)
  {
    case ezFileOpenMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case ezFileOpenMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case ezFileOpenMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, ezFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
    return EZ_FAILURE;


  if (FileShareMode == ezFileShareMode::Default)
  {
    if (OpenMode == ezFileOpenMode::Read)
    {
      FileShareMode = ezFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = ezFileShareMode::Exclusive;
    }
  }

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP) // UWP does not support these functions

  const int iSharedMode = (FileShareMode == ezFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;

  const int fileNo = fileno(m_FileData.m_pFileHandle);
  if (flock(fileNo, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    // error, could not get a lock

    fclose(m_FileData.m_pFileHandle);
    m_FileData.m_pFileHandle = nullptr;
    return EZ_FAILURE;
  }
#endif

  // lock will be released automatically when the file is closed
  return EZ_SUCCESS;
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

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const
{
#if EZ_ENABLED(EZ_USE_OLD_POSIX_FUNCTIONS)
  switch (Pos)
  {
    case ezFileSeekMode::FromStart:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case ezFileSeekMode::FromEnd:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case ezFileSeekMode::FromCurrent:
      EZ_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case ezFileSeekMode::FromStart:
      EZ_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case ezFileSeekMode::FromEnd:
      EZ_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case ezFileSeekMode::FromCurrent:
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
#  define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
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

ezResult ezOSFile::InternalDeleteDirectory(const char* szDirectory)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  int iRes = _rmdir(szDirectory);
#else
  int iRes = rmdir(szDirectory);
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
  // Note that this is espcially relevant for calls to ezOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
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
#  if EZ_ENABLED(EZ_PLATFORM_OSX)

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
#  elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
    {
      ezJniAttachment attachment;

      ezJniString packagePath = attachment.GetActivity().Call<ezJniString>("getPackageCodePath");
      // By convention, android requires assets to be placed in the 'Assets' folder
      // inside the apk thus we use that as our SDK root.
      ezStringBuilder sTemp = packagePath.GetData();
      sTemp.AppendPath("Assets");
      s_Path = sTemp;
    }
#  else
    char result[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", result, PATH_MAX);
    ezStringBuilder path(ezStringView(result, result + length));
    s_Path = path.GetFileDirectory();
#  endif
  }

  return s_Path.GetData();
}

ezString ezOSFile::GetUserDataFolder(const char* szSubFolder)
{
  if (s_UserDataPath.IsEmpty())
  {
#  if EZ_ENABLED(EZ_PLATFORM_ANDROID)
    android_app* app = ezAndroidUtils::GetAndroidApp();
    s_UserDataPath = app->activity->internalDataPath;
#  else
    s_UserDataPath = getenv("HOME");

    if (s_UserDataPath.IsEmpty())
      s_UserDataPath = getpwuid(getuid())->pw_dir;
#  endif
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
#  if EZ_ENABLED(EZ_PLATFORM_ANDROID)
    ezJniAttachment attachment;

    ezJniObject cacheDir = attachment.GetActivity().Call<ezJniObject>("getCacheDir");
    ezJniString path = cacheDir.Call<ezJniString>("getPath");
    s_TempDataPath = path.GetData();
#  else
    s_TempDataPath = GetUserDataFolder(".cache").GetData();
#  endif
  }

  ezStringBuilder s = s_TempDataPath;
  s.AppendPath(szSubFolder);
  s.MakeCleanPath();
  return s;
}

const ezString ezOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  ezStringBuilder clean = getcwd(tmp, EZ_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#  if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

ezFileSystemIterator::ezFileSystemIterator()
{
}

ezFileSystemIterator::~ezFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    closedir(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool ezFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

namespace
{
  ezResult UpdateCurrentFile(ezFileStats& curFile, const ezStringBuilder& curPath, DIR* hSearch, const ezString& wildcardSearch)
  {
    struct dirent* hCurrentFile = readdir(hSearch);
    if (hCurrentFile == nullptr)
      return EZ_FAILURE;

    if (!wildcardSearch.IsEmpty())
    {
      while (fnmatch(wildcardSearch.GetData(), hCurrentFile->d_name, FNM_NOESCAPE) != 0)
      {
        hCurrentFile = readdir(hSearch);
        if (hCurrentFile == nullptr)
          return EZ_FAILURE;
      }
    }

    ezStringBuilder absFileName = curPath;
    absFileName.AppendPath(hCurrentFile->d_name);

    struct stat fileStat = {};
    stat(absFileName.GetData(), &fileStat);

    curFile.m_uiFileSize = fileStat.st_size;
    curFile.m_bIsDirectory = hCurrentFile->d_type == DT_DIR;
    curFile.m_sParentPath = curPath;
    curFile.m_sName = hCurrentFile->d_name;
    curFile.m_LastModificationTime.SetInt64(fileStat.st_mtime, ezSIUnitOfTime::Second);

    return EZ_SUCCESS;
  }
} // namespace

void ezFileSystemIterator::StartSearch(const char* szSearchStart, ezBitflags<ezFileSystemIteratorFlags> flags /*= ezFileSystemIteratorFlags::All*/)
{
  EZ_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  ezStringBuilder sSearch = szSearchStart;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // Remove a trailing slash if any
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  if (flags.IsSet(ezFileSystemIteratorFlags::Recursive) == true && bHasWildcard == true)
  {
    EZ_ASSERT_DEV(false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");
    return;
  }

  if (bHasWildcard)
  {
    m_Data.m_wildcardSearch = sSearch.GetFileNameAndExtension();
    m_sCurPath = sSearch.GetFileDirectory();
  }
  else
  {
    m_Data.m_wildcardSearch.Clear();
    m_sCurPath = sSearch;
  }

  EZ_ASSERT_DEV(m_sCurPath.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  DIR* hSearch = opendir(m_sCurPath.GetData());

  if (hSearch == nullptr)
    return;

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Failed())
  {
    return;
  }

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

void ezFileSystemIterator::Next()
{
  while (true)
  {
    const ezInt32 res = InternalNext();

    if (res == EZ_SUCCESS)
      return;

    if (res == EZ_FAILURE)
      return;
  }
}


ezInt32 ezFileSystemIterator::InternalNext()
{
  constexpr ezInt32 CallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return EZ_FAILURE;

  if (m_Flags.IsSet(ezFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName.GetData());

    DIR* hSearch = opendir(m_sCurPath.GetData());

    if (hSearch != nullptr && UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Succeeded())
    {
      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return CallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFolders))
          return CallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFiles))
          return CallInternalNext;
      }

      return EZ_SUCCESS;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, m_Data.m_Handles.PeekBack(), m_Data.m_wildcardSearch).Failed())
  {
    // nothing found in this directory anymore
    closedir(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return EZ_FAILURE;

    m_sCurPath.PathParentDirectory();

    return CallInternalNext;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return CallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFolders))
      return CallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(ezFileSystemIteratorFlags::ReportFiles))
      return CallInternalNext;
  }

  return EZ_SUCCESS;
}

void ezFileSystemIterator::SkipFolder()
{
  EZ_ASSERT_DEBUG(m_Flags.IsSet(ezFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  EZ_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(ezFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(ezFileSystemIteratorFlags::Recursive);
}

#  endif

#endif // EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
