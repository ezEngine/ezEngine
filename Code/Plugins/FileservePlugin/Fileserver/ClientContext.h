#pragma once

#include <FileservePlugin/FileservePluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

enum class ezFileserveFileState
{
  None = 0,
  NonExistant = 1,
  NonExistantEither = 2,
  SameTimestamp = 3,
  SameHash = 4,
  Different = 5,
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
    bool m_bMounted = false;
  };

  struct FileStatus
  {
    ezInt64 m_iTimestamp = -1;
    ezUInt64 m_uiHash = 0;
    ezUInt64 m_uiFileSize = 0;
  };

  ezFileserveFileState GetFileStatus(ezUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status, ezDynamicArray<ezUInt8>& out_FileContent, bool bForceThisDataDir) const;

  bool m_bLostConnection = false;
  ezUInt32 m_uiApplicationID = 0;
  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

