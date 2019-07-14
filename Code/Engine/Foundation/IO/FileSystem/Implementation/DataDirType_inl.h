#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline ezDataDirectoryReaderWriterBase::ezDataDirectoryReaderWriterBase(ezInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirectory = nullptr;
  m_bIsReader = bIsReader;
}

inline ezResult ezDataDirectoryReaderWriterBase::Open(const char* szResourcePath, ezDataDirectoryType* pDataDirectory)
{
  m_pDataDirectory = pDataDirectory;
  m_sFilePath = szResourcePath;

  return InternalOpen();
}

inline const ezString128& ezDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline ezDataDirectoryType* ezDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirectory;
}

