#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct ezOSFileData
{
  ezOSFileData() { m_pFileHandle = nullptr; }

  FILE* m_pFileHandle;
};

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

struct ezFileIterationData
{
  // This is storing DIR*, which we can't forward declare
  ezHybridArray<void*, 16> m_Handles;
  ezString m_wildcardSearch;
};

#endif

/// \endcond
