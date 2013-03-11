#pragma once

#include <Foundation/Basics.h>

#if EZ_USE_POSIX_FILE_API == 0

struct ezOSFileData
{
  ezOSFileData()
  {
    m_pFileHandle = INVALID_HANDLE_VALUE;
  }

  HANDLE m_pFileHandle;
};

struct ezFileIterationData
{
  ezHybridArray<HANDLE, 16> m_Handles;
};

#endif