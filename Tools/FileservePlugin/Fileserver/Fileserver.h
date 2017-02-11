#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Configuration/Singleton.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class ezNetworkMessage;

struct ezFileserverEvent
{
  enum class Type
  {
    None,
    ServerStarted,
    ServerStopped,
    ConnectedNewClient,
    MountDataDir,
    UnmountDataDir,
    FileRequest,
    FileTranser,
    FileTranserFinished,
    FileDeleteRequest,
    FileUploading,
    FileUploadedFinished,
  };

  Type m_Type = Type::None;
  const char* m_szPath = nullptr;
  const char* m_szDataDirRootName = nullptr;
  ezUInt32 m_uiSizeTotal = 0;
  ezUInt32 m_uiSentTotal = 0;
  ezFileserveFileState m_FileState = ezFileserveFileState::None;
};

class EZ_FILESERVEPLUGIN_DLL ezFileserver
{
  EZ_DECLARE_SINGLETON(ezFileserver);

public:
  ezFileserver();

  void StartServer(ezUInt16 uiPort = 1042);
  void StopServer();
  bool UpdateServer();

  ezEvent<const ezFileserverEvent&> m_Events;

private:
  ezFileserveClientContext& DetermineClient(ezNetworkMessage &msg);
  void NetworkMsgHandler(ezNetworkMessage& msg);
  void HandleMountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleUnmountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleDeleteFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleUploadFileHeader(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleUploadFileTransfer(ezFileserveClientContext& client, ezNetworkMessage &msg);
  void HandleUploadFileFinished(ezFileserveClientContext& client, ezNetworkMessage &msg);

  ezHashTable<ezUInt32, ezFileserveClientContext> m_Clients;
  ezUniquePtr<ezNetworkInterface> m_Network;
  ezDynamicArray<ezUInt8> m_SendToClient; // ie. 'downloads' from server to client
  ezDynamicArray<ezUInt8> m_SentFromClient; // ie. 'uploads' from client to server
  ezStringBuilder m_sCurFileUpload;
  ezUuid m_FileUploadGuid;
  ezUInt32 m_uiFileUploadSize;
  ezUInt16 m_FileUploadDataDir;
};

