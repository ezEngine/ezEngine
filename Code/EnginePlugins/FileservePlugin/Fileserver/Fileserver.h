#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class ezRemoteMessage;

struct ezFileserverEvent
{
  enum class Type
  {
    None,
    ServerStarted,
    ServerStopped,
    ClientConnected,
    ClientReconnected, // connected again after a disconnect
    ClientDisconnected,
    MountDataDir,
    MountDataDirFailed,
    UnmountDataDir,
    FileDownloadRequest,
    FileDownloading,
    FileDownloadFinished,
    FileDeleteRequest,
    FileUploadRequest,
    FileUploading,
    FileUploadFinished,
    AreYouThereRequest,
  };

  Type m_Type = Type::None;
  ezUInt32 m_uiClientID = 0;
  const char* m_szName = nullptr;
  const char* m_szPath = nullptr;
  const char* m_szRedirectedPath = nullptr;
  ezUInt32 m_uiSizeTotal = 0;
  ezUInt32 m_uiSentTotal = 0;
  ezFileserveFileState m_FileState = ezFileserveFileState::None;
};

/// \brief A file server allows to serve files from a host PC to another process that is potentially on another device.
///
/// This is mostly useful for mobile devices, that do not have access to the data on the development machine.
/// Typically every change to a file would require packaging the app and deploying it to the device again.
/// Fileserve allows to only deploy a very lean application and instead get all asset data directly from a host PC.
/// This also allows to modify data on the PC and reload the data in the running application without delay.
///
/// A single file server can serve multiple clients. However, to mount "special directories" (see ezFileSystem) the server
/// needs to know what local path to map them to (it uses the configuration on ezFileSystem).
/// That means it cannot serve two clients that require different settings for the same special directory.
///
/// The port on which the server connects to clients can be configured through the command line option "-fs_port X"
class EZ_FILESERVEPLUGIN_DLL ezFileserver
{
  EZ_DECLARE_SINGLETON(ezFileserver);

public:
  ezFileserver();

  /// \brief Starts listening for client connections. Uses the configured port.
  void StartServer();

  /// \brief Disconnects all clients.
  void StopServer();

  /// \brief Has to be executed regularly to serve clients and keep the connection alive.
  bool UpdateServer();

  /// \brief Whether the server was started.
  bool IsServerRunning() const;

  /// \brief Overrides the current port setting. May only be called when the server is currently not running.
  void SetPort(ezUInt16 uiPort);

  /// \brief Returns the currently set port. If the command line option "-fs_port X" was used, this will return that value, otherwise the default is 1042.
  ezUInt16 GetPort() const { return m_uiPort; }

  /// \brief The server broadcasts events about its activity
  ezEvent<const ezFileserverEvent&> m_Events;

  /// \brief Broadcasts to all clients that they should reload their resources
  void BroadcastReloadResourcesCommand();

  static ezResult SendConnectionInfo(const char* szClientAddress, ezUInt16 uiMyPort, const ezArrayPtr<ezStringBuilder>& MyIPs, ezTime timeout = ezTime::Seconds(10));

private:
  void NetworkEventHandler(const ezRemoteEvent& e);
  ezFileserveClientContext& DetermineClient(ezRemoteMessage &msg);
  void NetworkMsgHandler(ezRemoteMessage& msg);
  void HandleMountRequest(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleUnmountRequest(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleFileRequest(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleDeleteFileRequest(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleUploadFileHeader(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleUploadFileTransfer(ezFileserveClientContext& client, ezRemoteMessage &msg);
  void HandleUploadFileFinished(ezFileserveClientContext& client, ezRemoteMessage &msg);

  ezHashTable<ezUInt32, ezFileserveClientContext> m_Clients;
  ezUniquePtr<ezRemoteInterface> m_Network;
  ezDynamicArray<ezUInt8> m_SendToClient; // ie. 'downloads' from server to client
  ezDynamicArray<ezUInt8> m_SentFromClient; // ie. 'uploads' from client to server
  ezStringBuilder m_sCurFileUpload;
  ezUuid m_FileUploadGuid;
  ezUInt32 m_uiFileUploadSize;
  ezUInt16 m_uiPort = 1042;
};

