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

  /// \brief Searches all data directories of \a client for the requested file. On success the full path is returned.
  ezResult FindFileInDataDirs(ezUInt16 uiDataDirID, const char* szRequestedFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, const ezFileserveClientContext::DataDir** ppDataDir) const;

  ezUInt32 m_uiApplicationID = 0;
  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

