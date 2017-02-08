#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

enum class ezFileserveFileState
{
  NonExistant = 0,
  SameTimestamp = 1,
  SameHash = 2,
  Different = 3,
};

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

  struct FileStatus
  {
    ezInt64 m_iTimestamp = -1;
    ezUInt64 m_uiHash = 0;
    ezUInt64 m_uiFileSize = 0;
  };

  ezFileserveFileState GetFileStatus(ezUInt16 uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status, ezDynamicArray<ezUInt8>& out_FileContent) const;

  ezUInt32 m_uiApplicationID = 0;
  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

