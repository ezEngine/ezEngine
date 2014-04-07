#pragma once

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

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
    pBuffer = ezMemoryUtils::AddByteOffsetConst(pBuffer, uiBatchBytes);
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
  return static_cast<ezUInt64>(ftello(m_FileData.m_pFileHandle));
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
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
}

bool ezOSFile::InternalExists(const char* szFile)
{
  FILE* pFile = fopen(szFile, "r");

  if (pFile == nullptr)
    return false;

  fclose(pFile);
  return true;
}

ezResult ezOSFile::InternalDeleteFile(const char* szFile)
{
  int iRes = unlink(szFile);
  
  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return EZ_SUCCESS;
  
  return EZ_FAILURE;
}

ezResult ezOSFile::InternalCreateDirectory(const char* szDirectory)
{
  // handle drive letters as always successful
  if (ezStringUtils::GetCharacterCount(szDirectory) <= 1) // '/'
    return EZ_SUCCESS;
  
  int iRes = mkdir(szDirectory, 0777);
  
  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return EZ_SUCCESS;
    
  return EZ_FAILURE;
}

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
ezResult ezOSFile::InternalGetFileStats(const char* szFileOrFolder, ezFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(szFileOrFolder, &tempStat);
  
  if (iRes != 0)
    return EZ_FAILURE;
  
  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sFileName = "";
  out_Stats.m_LastModificationTime.SetInt64(tempStat.st_mtime, ezSIUnitOfTime::Second);
  
  return EZ_SUCCESS;
}
#endif

const char* ezOSFile::GetApplicationDirectory()
{
  #warning Not yet implemented.
  return nullptr;
}

