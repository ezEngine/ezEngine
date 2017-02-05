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

  void EnsureConnected();

  void UpdateClient();

  void MountDataDirectory(const char* szDataDir);

  void NetworkMsgHandler(ezNetworkMessage& msg);

  void DownloadFile(const char* szFile);

private:
  ezString m_sFileserveCacheFolder;
  bool m_bDownloading = false;
  ezUInt16 m_uiCurFileDownload = 0;
  ezString m_sCurrentFileDownload;
  ezUniquePtr<ezNetworkInterface> m_Network;
  ezDynamicArray<ezUInt8> m_Download;
};

