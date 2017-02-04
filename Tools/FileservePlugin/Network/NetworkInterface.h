#pragma once

#include <FileservePlugin/Plugin.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>
#include <FileservePlugin/Network/NetworkMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>

enum class ezNetworkMode
{
  None,
  Server,
  Client
};

enum class ezNetworkTransmitMode
{
  Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
  Unreliable, ///< Messages are sent at most once, if they get lost, they are not resent. If it is known beforehand, that not receiver exists, they are dropped without sending them at all.
};

struct ezNetworkEvent
{
  enum Type
  {
    ConnectedToClient,        ///< brief Sent whenever a new connection to a client has been established.
    ConnectedToServer,        ///< brief Sent whenever a connection to the server has been established.
    DisconnectedFromClient,   ///< Sent every time the connection to a client is dropped
    DisconnectedFromServer,   ///< Sent when the connection to the server has been lost
  };

  Type m_Type;
};

typedef ezDelegate<void(ezNetworkMessage&)> ezNetworkMessageHandler;

struct EZ_FILESERVEPLUGIN_DLL ezNetworkMessageQueue
{
  ezNetworkMessageHandler m_MessageHandler;
  ezDeque<ezNetworkMessage> m_MessageQueue;
};

class EZ_FILESERVEPLUGIN_DLL ezNetworkInterface
{
public:
  ~ezNetworkInterface();

  ezResult StartServer(ezUInt32 uiConnectionToken, ezUInt16 uiPort, bool bStartUpdateThread = true);
  ezResult ConnectToServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread = true);

  bool IsConnectedToServer() const { return m_uiConnectedToServerWithID != 0; }
  bool IsConnectedToClients() const { return m_iConnectionsToClients > 0; }
  bool IsConnectedToOther() const { return IsConnectedToServer() || IsConnectedToClients(); }

  void ShutdownConnection();

  ezTime GetPingToServer() const { return m_PingToServer; }
  void UpdatePingToServer();
  void UpdateNetwork();

  ezUInt16 GetPort() const { return m_uiPort; }
  ezNetworkMode GetNetworkMode() const { return m_NetworkMode; }

  ezMutex& GetMutex() const { return m_Mutex; }

  ezResult Transmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data);
  void SendShortMessage(ezUInt32 uiSystemID, ezUInt32 uiMsgID);
  void Send(ezNetworkTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data);
  void Send(ezNetworkTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData = nullptr, ezUInt32 uiDataBytes = 0);
  void Send(ezNetworkTransmitMode tm, ezNetworkMessage& msg);

  const ezString& GetServerInfoName() const { return m_ServerInfoName; }
  const ezString& GetServerInfoIP() const { return m_ServerInfoIP; }

  ezUInt32 GetServerID() const { return m_uiConnectedToServerWithID; }

  void SetMessageHandler(ezUInt32 uiSystemID, ezNetworkMessageHandler messageHandler);

  void ExecuteMessageHandlers(ezUInt32 uiSystem);
  void ExecuteAllMessageHandlers();

  ezEvent<const ezNetworkEvent&> m_NetworkEvents;

protected:
  virtual ezResult InternalCreateConnection(ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress) = 0;
  virtual void InternalShutdownConnection() = 0;
  virtual void InternalUpdateNetwork() = 0;
  virtual ezTime InternalGetPingToServer() = 0;
  virtual ezResult InternalTransmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) = 0;
  virtual ezResult DetermineTargetAddress(const char* szConnectTo, ezUInt32& out_IP, ezUInt16& out_Port);
  void StartUpdateThread();
  void StopUpdateThread();
  ezUInt32 GetApplicationID() const { return m_uiApplicationID; }
  ezUInt32 GetConnectionToken() const { return m_uiConnectionToken; }

  void ReportConnectionToServer(ezUInt32 uiServerID);
  void ReportConnectionToClient();
  void ReportDisconnectedFromServer();
  void ReportDisconnectedFromClient();
  void ReportMessage(ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data);

  ezString m_ServerInfoName;
  ezString m_ServerInfoIP;

private:
  ezResult CreateConnection(ezUInt32 uiConnectionToken, ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress, bool bStartUpdateThread);
  void ExecuteMessageHandlersForQueue(ezNetworkMessageQueue& queue);

  mutable ezMutex m_Mutex;
  class ezNetworkThread* m_pUpdateThread = nullptr;
  ezUInt16 m_uiPort = 0;
  ezNetworkMode m_NetworkMode = ezNetworkMode::None;
  ezTime m_PingToServer;
  ezUInt32 m_uiApplicationID = 0; // sent when connecting to identify the sending instance
  ezUInt32 m_uiConnectionToken = 0;
  ezUInt32 m_uiConnectedToServerWithID = 0;
  ezInt32 m_iConnectionsToClients = 0;
  ezDynamicArray<ezUInt8> m_TempSendBuffer;
  ezHashTable<ezUInt32, ezNetworkMessageQueue> m_MessageQueues;
};

class EZ_FILESERVEPLUGIN_DLL ezNetworkThread : public ezThread
{
public:
  ezNetworkThread();

  ezNetworkInterface* m_pNetwork = nullptr;
  volatile bool m_bKeepRunning = true;

private:
  virtual ezUInt32 Run();
};



