#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Configuration/Singleton.h>

namespace ezDataDirectory
{
  class FileserveType;
}

/// \brief Singleton that represents the client side part of a fileserve connection
class EZ_FILESERVEPLUGIN_DLL ezFileserveClient
{
  EZ_DECLARE_SINGLETON(ezFileserveClient);

public:
  ezFileserveClient();
  ~ezFileserveClient();

  static bool s_bEnableFileserve;

  void SetServerConnectionAddress(const char* szAddress) { m_sServerConnectionAddress = szAddress; }
  ezResult EnsureConnected();
  void UpdateClient();

private:
  friend class ezDataDirectory::FileserveType;

  struct FileCacheStatus
  {
    ezUInt16 m_uiDataDir = 0xffff;
    ezInt64 m_TimeStamp = 0;
    ezUInt64 m_FileHash = 0;
    ezTime m_LastCheck;
  };

  struct DataDir
  {
    //ezString m_sRootName;
    //ezString m_sPathOnClient;
    ezString m_sMountPoint;
  };

  ezUInt16 MountDataDirectory(const char* szDataDir, const char* szRootName);
  void ComputeDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const;
  void BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sFullPathMeta) const;
  void GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const;
  void NetworkMsgHandler(ezNetworkMessage& msg);
  void HandleFileTransferMsg(ezNetworkMessage &msg);
  void HandleFileTransferFinishedMsg(ezNetworkMessage &msg);
  ezResult DownloadFile(ezUInt16 uiDataDirID, const char* szFile);
  void DetermineCacheStatus(ezUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;

  ezString m_sServerConnectionAddress;
  ezString m_sFileserveCacheFolder;
  ezString m_sFileserveCacheMetaFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  ezUInt16 m_uiCurFileRequestID = 0;
  ezUInt16 m_uiCurFileRequestDataDir = 0;
  ezStringBuilder m_sCurFileRequest;
  ezUniquePtr<ezNetworkInterface> m_Network;
  ezDynamicArray<ezUInt8> m_Download;
  ezMap<ezString, FileCacheStatus> m_CachedFileStatus;


  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

