#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline ezDataDirectoryReaderWriterBase::ezDataDirectoryReaderWriterBase(ezInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirType = nullptr;
  m_bIsReader = bIsReader;
}

inline ezResult ezDataDirectoryReaderWriterBase::Open(ezStringView sFile, ezDataDirectoryType* pDataDirectory, ezFileShareMode::Enum fileShareMode)
{
  m_pDataDirType = pDataDirectory;
  m_sFilePath = sFile;

  return InternalOpen(fileShareMode);
}

inline const ezString128& ezDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline ezDataDirectoryType* ezDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirType;
}
