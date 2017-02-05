#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

class EZ_FILESERVEPLUGIN_DLL ezFileserveClientContext
{
public:

  struct DataDir
  {
    ezString m_sRootName;
    ezString m_sPathOnClient;
    ezString m_sPathOnServer;
    ezString m_sMountPoint;
  };

  ezUInt32 m_uiApplicationID = 0;
  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

