#pragma once

bool ezOSFile::InternalOpen(const char* szFile, ezFileMode::Enum OpenMode)
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

    // in append mode we need to set the file pointer to the end explicitely, otherwise GetFilePosition might return 0 the first time
    if (m_FileData.m_pFileHandle != NULL)
      InternalSetFilePosition(0, ezFilePos::FromEnd);

    break;
  }

  return m_FileData.m_pFileHandle != NULL;
}

void ezOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

bool ezOSFile::InternalWrite(const void* pBuffer, ezUInt64 uiBytes)
{
  const ezUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
      return false;

    uiBytes -= uiBatchBytes;
    pBuffer = ezMemoryUtils::AddByteOffsetConst(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const ezUInt32 uiBytes32 = static_cast<ezUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
      return false;
  }

  return true;
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
  return static_cast<ezUInt64>(_ftelli64(m_FileData.m_pFileHandle));
}

void ezOSFile::InternalSetFilePosition(ezInt64 iDistance, ezFilePos::Enum Pos) const
{
  switch(Pos)
  {
  case ezFilePos::FromStart:
    EZ_VERIFY(_fseeki64(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
    break;
  case ezFilePos::FromEnd:
    EZ_VERIFY(_fseeki64(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
    break;
  case ezFilePos::FromCurrent:
    EZ_VERIFY(_fseeki64(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
    break;
  }
}
