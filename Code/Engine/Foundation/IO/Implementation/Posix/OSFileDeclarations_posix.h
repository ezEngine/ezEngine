#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct ezOSFileData
{
  ezOSFileData()
  {
    m_pFileHandle = nullptr;
  }

  FILE* m_pFileHandle;
};


/// \endcond

