#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef EZ_POSIX_FILE_USEOLDAPI
#  include <direct.h>
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

ezResult ezOSFile::InternalOpen(ezStringView sFile, ezFileOpenMode::Enum OpenMode, ezFileShareMode::Enum FileShareMode)
{
  ezStringBuilder sFileCopy = sFile;
  const char* szFile = sFileCopy;

#ifndef EZ_POSIX_FILE_USEOLDAPI // UWP does not support these functions
  int fd = -1;
  switch (OpenMode)
  {
    // O_CLOEXEC = don't forward to child processes
    case ezFileOpenMode::Read:
      fd = open(szFile, O_RDONLY | O_CLOEXEC);
      break;
    case ezFileOpenMode::Write:
    case ezFileOpenMode::Append:
      fd = open(szFile, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
      break;
    default:
      break;
  }

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

  if (fd == -1)
  {
    return EZ_FAILURE;
  }

  const int iSharedMode = (FileShareMode == ezFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;
  const ezTime sleepTime = ezTime::MakeFromMilliseconds(20);
  ezInt32 iRetries = m_bRetryOnSharingViolation ? 20 : 1;

  while (flock(fd, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    int errorCode = errno;
    iRetries--;
    if (iRetries == 0 || errorCode != EWOULDBLOCK)
    {
      // error, could not get a lock
      ezLog::Error("Failed to get a {} lock for file {}, error {}", (FileShareMode == ezFileShareMode::Exclusive) ? "Exculsive" : "Shared", szFile, errno);
      close(fd);
      return EZ_FAILURE;
    }
    ezThreadUtils::Sleep(sleepTime);
  }

  switch (OpenMode)
  {
    case ezFileOpenMode::Read:
      m_FileData.m_pFileHandle = fdopen(fd, "rb");
      break;
    case ezFileOpenMode::Write:
      if (ftruncate(fd, 0) < 0)
      {
        close(fd);
        return EZ_FAILURE;
      }
      m_FileData.m_pFileHandle = fdopen(fd, "wb");
      break;
    case ezFileOpenMode::Append:
      m_FileData.m_pFileHandle = fdopen(fd, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, ezFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
  {
    close(fd);
  }

#else
  EZ_IGNORE_UNUSED(FileShareMode);

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
#endif

  if (m_FileData.m_pFileHandle == nullptr)
  {
    return EZ_FAILURE;
  }

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
    {
      ezLog::Error("fwrite 1GB failed for '{}'", m_sFileName);
      return EZ_FAILURE;
    }

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
    {
      ezLog::Error("fwrite failed for '{}'", m_sFileName);
      return EZ_FAILURE;
    }
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
#ifdef EZ_POSIX_FILE_USEOLDAPI
  return static_cast<ezUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<ezUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFileSeekMode::Enum Pos) const
{
#ifdef EZ_POSIX_FILE_USEOLDAPI
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

// this might not be defined on Windows
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

bool ezOSFile::InternalExistsFile(ezStringView sFile)
{
  struct stat sb;
  return (stat(ezString(sFile), &sb) == 0 && !S_ISDIR(sb.st_mode));
}

bool ezOSFile::InternalExistsDirectory(ezStringView sDirectory)
{
  struct stat sb;
  return (stat(ezString(sDirectory), &sb) == 0 && S_ISDIR(sb.st_mode));
}

ezResult ezOSFile::InternalDeleteFile(ezStringView sFile)
{
#ifdef EZ_POSIX_FILE_USEWINDOWSAPI
  int iRes = _unlink(ezString(sFile));
#else
  int iRes = unlink(ezString(sFile));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezResult ezOSFile::InternalDeleteDirectory(ezStringView sDirectory)
{
#ifdef EZ_POSIX_FILE_USEWINDOWSAPI
  int iRes = _rmdir(ezString(sDirectory));
#else
  int iRes = rmdir(ezString(sDirectory));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezResult ezOSFile::InternalCreateDirectory(ezStringView sDirectory)
{
  // handle drive letters as always successful
  if (ezStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 1) // '/'
    return EZ_SUCCESS;

#ifdef EZ_POSIX_FILE_USEWINDOWSAPI
  int iRes = _mkdir(ezString(sDirectory));
#else
  int iRes = mkdir(ezString(sDirectory), 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return EZ_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to ezOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
  if (errno == EACCES && InternalExistsDirectory(sDirectory))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezResult ezOSFile::InternalMoveFileOrDirectory(ezStringView sDirectoryFrom, ezStringView sDirectoryTo)
{
  if (rename(ezString(sDirectoryFrom), ezString(sDirectoryTo)) != 0)
  {
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

#  ifndef EZ_POSIX_FILE_NOINTERNALGETFILESTATS

ezResult ezOSFile::InternalGetFileStats(ezStringView sFileOrFolder, ezFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(ezString(sFileOrFolder), &tempStat);

  if (iRes != 0)
    return EZ_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = ezPathUtils::GetFileNameAndExtension(sFileOrFolder); // no OS support, so just pass it through
#    ifdef __USE_XOPEN2K8
  out_Stats.m_LastModificationTime = ezTimestamp::MakeFromInt(tempStat.st_mtim.tv_sec * 1000000000ull + tempStat.st_mtim.tv_nsec, ezSIUnitOfTime::Nanosecond);
#    else
  out_Stats.m_LastModificationTime = ezTimestamp::MakeFromInt(tempStat.st_mtime, ezSIUnitOfTime::Second);
#    endif
  return EZ_SUCCESS;
}

#  endif

#endif

#ifndef EZ_POSIX_FILE_NOGETAPPLICATIONPATH

ezStringView ezOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    char result[PATH_MAX];
    size_t length = readlink("/proc/self/exe", result, PATH_MAX);
    s_sApplicationPath = ezStringView(result, result + length);
  }

  return s_sApplicationPath;
}

#endif

#ifndef EZ_POSIX_FILE_NOGETUSERDATAFOLDER

ezString ezOSFile::GetUserDataFolder(ezStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
  }

  ezStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef EZ_POSIX_FILE_NOGETTEMPDATAFOLDER

ezString ezOSFile::GetTempDataFolder(ezStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    s_sTempDataPath = GetUserDataFolder(".cache").GetData();
  }

  ezStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef EZ_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

ezString ezOSFile::GetUserDocumentsFolder(ezStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
  }

  ezStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef EZ_POSIX_FILE_NOGETCURRENTWORKINGDIRECTORY

const ezString ezOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  ezStringBuilder clean = getcwd(tmp, EZ_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#endif
