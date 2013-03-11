#pragma once

#include <Foundation/Basics.h>

struct ezOSFileData
{
  ezOSFileData()
  {
    m_pFileHandle = NULL;
  }

  FILE* m_pFileHandle;
};

