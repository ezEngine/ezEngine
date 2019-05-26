#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Avoid conflicts with windows.h
#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#if EZ_DISABLED(EZ_USE_POSIX_FILE_API)

#include <Foundation/Basics/Platform/Win/MinWindows.h>

struct ezOSFileData
{
  ezOSFileData()
  {
    m_pFileHandle = EZ_WINDOWS_INVALID_HANDLE_VALUE;
  }

  ezMinWindows::HANDLE m_pFileHandle;
};

struct ezFileIterationData
{
  ezHybridArray<ezMinWindows::HANDLE, 16> m_Handles;
};

#endif

/// \endcond

