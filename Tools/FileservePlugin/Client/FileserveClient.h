#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Configuration/Singleton.h>

class EZ_FILESERVEPLUGIN_DLL ezFileserveClient
{
  EZ_DECLARE_SINGLETON(ezFileserveClient);

public:
  ezFileserveClient();
  ~ezFileserveClient();

  ezResult EnsureConnected();

  void UpdateClient();

  void MountDataDirectory(const char* szDataDir, const char* szRootName);

  void GetDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const;
  void GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath) const;

  void NetworkMsgHandler(ezNetworkMessage& msg);

  void HandleFileTransferMsg(ezNetworkMessage &msg);

  ezResult DownloadFile(const char* szFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath);

  bool IsFileCached(const char* szFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath) const;
  void CreateFileserveCachePath(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath) const;

private:
  ezString m_sFileserveCacheFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  ezUInt16 m_uiCurFileDownload = 0;
  ezString m_sCurrentFileDownload;
  ezUniquePtr<ezNetworkInterface> m_Network;
  ezDynamicArray<ezUInt8> m_Download;

  struct DataDir
  {
    ezString m_sRootName;
    ezString m_sPathOnClient;
    ezString m_sMountPoint;
  };

  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

