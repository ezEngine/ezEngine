#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

class EZ_FILESERVEPLUGIN_DLL ezFileserveClientContext
{
public:

  ezUInt32 m_uiApplicationID = 0;
  ezHybridArray<ezString, 8> m_MountedDataDirs;
};

