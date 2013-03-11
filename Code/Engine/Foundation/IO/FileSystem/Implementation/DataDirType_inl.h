#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline ezDataDirectory_ReaderWriter_Base::ezDataDirectory_ReaderWriter_Base(bool bIsReader)
{
  m_pDataDirectory = NULL;
  m_bIsReader = bIsReader; 
}

inline ezResult ezDataDirectory_ReaderWriter_Base::Open(const char* szResourcePath, ezDataDirectoryType* pDataDirectory)
{
  m_pDataDirectory = pDataDirectory;
  m_sFilePath = szResourcePath;

  if (InternalOpen() == EZ_SUCCESS)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

inline const ezString128& ezDataDirectory_ReaderWriter_Base::GetFilePath() const 
{ 
  return m_sFilePath; 
}

inline ezDataDirectoryType* ezDataDirectory_ReaderWriter_Base::GetDataDirectory() const 
{ 
  return m_pDataDirectory; 
}

