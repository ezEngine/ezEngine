#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct ezOSFileData
{
  ezOSFileData() { m_pFileHandle = nullptr; }

  FILE* m_pFileHandle;
};

extern "C"
{
  typedef struct __dirstream DIR;
}

struct ezFileIterationData
{
  ezHybridArray<DIR*, 16> m_Handles;
  ezString m_wildcardSearch;
};

/// \endcond
