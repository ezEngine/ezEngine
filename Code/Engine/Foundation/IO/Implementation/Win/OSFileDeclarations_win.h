#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

#if EZ_DISABLED(EZ_USE_POSIX_FILE_API)

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

/// \endcond

