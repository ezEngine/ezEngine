#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Uuid.h>

namespace ezDataDirectory
{
  class FileserveType;
}

/// \brief Singleton that represents the client side part of a fileserve connection
///
/// Whether the fileserve plugin will be enabled is controled by ezFileserveClient::s_bEnableFileserve
/// By default this is on, but if switched off, the fileserve client functionality will be disabled.
/// ezFileserveClient will also switch its functionality off, if the command line argument "-fs_off" is specified.
/// If a program knows that it always wants to switch file serving off, it should either simply not load the plugin at all,
/// or it can inject that command line argument through ezCommandLineUtils. This should be done before application startup
/// and especially before any data directories get mounted.
///
/// The timeout for connecting to the server can be configured through the command line option "-fs_timeout seconds"
/// The server to connect to can be configured through command line option "-fs_server address".
/// The default address is "localhost:1042".
class EZ_FILESERVEPLUGIN_DLL ezFileserveClient
{
  EZ_DECLARE_SINGLETON(ezFileserveClient);

public:
  ezFileserveClient();
  ~ezFileserveClient();

  /// \brief Can be called at startup to go through multiple sources and search for a valid server address
  ///
  /// Ie. checks the command line, ezFileserve.txt in different directories, etc.
  /// For every potential IP it checks whether a fileserve connection could be established (e.g. tries to connect and
  /// checks whether the server answers). If a valid connection is found, the IP is stored internally and EZ_SUCCESS is returned.
  /// Call GetServerConnectionAddress() to retrieve the address.
  ///
  /// \param timeout Specifies the timeout for checking whether a server can be reached.
  ezResult SearchForServerAddress(ezTime timeout = ezTime::Seconds(20));

  /// \brief Waits for a Fileserver application to try to connect to this device and send its own information.
  ///
  /// This can be used when a device has no proper way to know the IP through which to connect to a Fileserver.
  /// Instead the device opens a server connection itself, and waits for the other side to try to connect to it.
  /// This typically means that a human has to manually input this device's IP on the host PC into the Fileserve application,
  /// thus enabling the exchange of connection information.
  /// Once this has happened, this function stores the valid server IP internally and returns with success.
  /// A subsequent call to EnsureConnected() should then succeed.
  ezResult WaitForServerInfo(ezTime timeout = ezTime::Seconds(60.0 * 5));

  /// \brief Stores the current connection info to a text file in the user data folder.
  ezResult SaveCurrentConnectionInfoToDisk() const;

  /// \brief Allows to disable the file serving functionality. Should be called before mounting data directories.
  ///
  /// Also achieved through the command line argument "-fs_off"
  static void DisabledFileserveClient() { s_bEnableFileserve = false; }

  /// \brief Returns the address through which the Fileserve client tried to connect with the server last.
  const char* GetServerConnectionAddress() { return m_sServerConnectionAddress; }

  /// \brief Can be called to ensure a fileserve connection. Otherwise automatically called when a data directory is mounted.
  ///
  /// The timeout defines how long the code will wait for a connection.
  /// Positive numbers are a regular timeout.
  /// A zero timeout means the application will wait indefinitely.
  /// A negative number means to either wait that time, or whatever was specified through the command-line.
  /// The timeout can be specified with the command line switch "-fs_timeout X" (in seconds).
  ezResult EnsureConnected(ezTime timeout = ezTime::Seconds(-5));

  /// \brief Needs to be called regularly to update the network. By default this is automatically called when the global event
  /// 'GameApp_UpdatePlugins' is fired, which is done by ezGameApplication.
  void UpdateClient();

  /// \brief Adds an address that should be tried for connecting with the server.
  void AddServerAddressToTry(const char* szAddress);

private:
  friend class ezDataDirectory::FileserveType;

  /// \brief True by default, can
  static bool s_bEnableFileserve;

  struct FileCacheStatus
  {
    ezInt64 m_TimeStamp = 0;
    ezUInt64 m_FileHash = 0;
    ezTime m_LastCheck;
  };

  struct DataDir
  {
    //ezString m_sRootName;
    //ezString m_sPathOnClient;
    ezString m_sMountPoint;
    bool m_bMounted = false;

    ezMap<ezString, FileCacheStatus> m_CacheStatus;
  };

  void DeleteFile(ezUInt16 uiDataDir, const char* szFile);
  ezUInt16 MountDataDirectory(const char* szDataDir, const char* szRootName);
  void UnmountDataDirectory(ezUInt16 uiDataDir);
  void ComputeDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const;
  void BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sFullPathMeta) const;
  void GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const;
  void NetworkMsgHandler(ezRemoteMessage& msg);
  void HandleFileTransferMsg(ezRemoteMessage &msg);
  void HandleFileTransferFinishedMsg(ezRemoteMessage &msg);
  void WriteMetaFile(ezStringBuilder sCachedMetaFile, ezInt64 iFileTimeStamp, ezUInt64 uiFileHash);
  void WriteDownloadToDisk(ezStringBuilder sCachedFile);
  ezResult DownloadFile(ezUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir);
  void DetermineCacheStatus(ezUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;
  void UploadFile(ezUInt16 uiDataDirID, const char* szFile, const ezDynamicArray<ezUInt8>& fileContent);
  void InvalidateFileCache(ezUInt16 uiDataDirID, const char* szFile, ezUInt64 uiHash);
  ezResult TryReadFileserveConfig(const char* szFile, ezStringBuilder& out_Result) const;
  ezResult TryConnectWithFileserver(const char* szAddress, ezTime timeout) const;
  void FillFileStatusCache(const char* szFile);

  mutable ezString m_sServerConnectionAddress;
  ezString m_sFileserveCacheFolder;
  ezString m_sFileserveCacheMetaFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  bool m_bWaitingForUploadFinished = false;
  ezUuid m_CurFileRequestGuid;
  ezStringBuilder m_sCurFileRequest;
  ezUniquePtr<ezRemoteInterface> m_Network;
  ezDynamicArray<ezUInt8> m_Download;
  ezTime m_CurrentTime;
  ezHybridArray<ezString, 4> m_TryServerAddresses;

  ezMap<ezString, ezUInt16> m_FileDataDir;

  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};

