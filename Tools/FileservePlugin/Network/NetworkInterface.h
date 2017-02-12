#pragma once

#include <FileservePlugin/Plugin.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>
#include <FileservePlugin/Network/NetworkMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>

/// \brief Whether the network is configured as a server or a client
enum class ezNetworkMode
{
  None,   ///< Network is shut down
  Server, ///< Network acts as a server. Can connect with multiple clients
  Client  ///< Network acts as a client. Can connect with exactly one server.
};

/// \brief Mode for transmitting messages
enum class ezNetworkTransmitMode
{
  Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
  Unreliable, ///< Messages are sent at most once, if they get lost, they are not resent. If it is known beforehand, that not receiver exists, they are dropped without sending them at all.
};

/// \brief Event type for connections
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
  ezUInt32 m_uiOtherAppID;
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

  /// \brief Exposes the mutex that is internally used to secure multi-threaded access
  ezMutex& GetMutex() const { return m_Mutex; }


  /// \name Connection
  ///@{

  /// \brief Starts the network interface as a server.
  ///
  /// \param uiConnectionToken Should be a unique sequence (e.g. 'EZPZ') to identify the purpose of this connection.
  /// Only server and clients with the same token will accept connections.
  /// \param uiPort The port over which the connection should run.
  /// \param bStartUpdateThread If true, a thread is started that will regularly call UpdateNetwork() and UpdatePingToServer().
  /// If false, this has to be called manually in regular intervals.
  ezResult StartServer(ezUInt32 uiConnectionToken, ezUInt16 uiPort, bool bStartUpdateThread = true);

  /// \brief Starts the network interface as a client. Tries to connect to the given address.
  ///
  /// This function immediately returns and no connection is guaranteed.
  /// \param uiConnectionToken Same as for StartServer()
  /// \param szAddress A network address. Could be "127.0.0.1", could be "localhost" or some other name that identifies the target.
  /// \param bStartUpdateThread Same as for StartServer()
  ///
  /// If this function succeeds, it still might not be connected to a server.
  /// Use WaitForConnectionToServer() to enforce a connection.
  ezResult ConnectToServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread = true);

  /// \brief Can only be called after ConnectToServer(). Updates the network in a loop until a connection is established, or the time has run out.
  ezResult WaitForConnectionToServer(ezTime timeout = ezTime::Seconds(10));

  /// \brief Closes the connection in an orderly fashion
  void ShutdownConnection();

  /// \brief Whether the client is connected to a server
  bool IsConnectedToServer() const { return m_uiConnectedToServerWithID != 0; }

  /// \brief Whether the server is connected to any client
  bool IsConnectedToClients() const { return m_iConnectionsToClients > 0; }

  /// \brief Whether the client or server is connected its counterpart
  bool IsConnectedToOther() const { return IsConnectedToServer() || IsConnectedToClients(); }

  /// \brief Whether the network is inactive, a client or a server
  ezNetworkMode GetNetworkMode() const { return m_NetworkMode; }

  /// \brief The port through which the connection was started
  ezUInt16 GetPort() const { return m_uiPort; }

  /// \brief Returns the own (random) application ID used to identify this instance
  ezUInt32 GetApplicationID() const { return m_uiApplicationID; }

  /// \brief Returns the connection token used to identify compatible servers/clients
  ezUInt32 GetConnectionToken() const { return m_uiConnectionToken; }

  ///@}

  /// \name Server Information
  ///@{

  /// \brief For the client to display the name of the server
  const ezString& GetServerInfoName() const { return m_ServerInfoName; }

  /// \brief For the client to display the IP of the server
  const ezString& GetServerInfoIP() const { return m_ServerInfoIP; }

  /// \brief Some random identifier, that allows to determine after a reconnect, whether the connected instance is still the same server
  ezUInt32 GetServerID() const { return m_uiConnectedToServerWithID; }

  /// \brief Returns the current ping to the server
  ezTime GetPingToServer() const { return m_PingToServer; }

  ///@}

  /// \name Updating the Network
  ///@{

  /// \brief If no update thread was spawned, this should be called to process messages
  void UpdateNetwork();

  /// \brief If no update thread was spawned, this should be called by clients to determine the ping
  void UpdatePingToServer();

  ///@}

  /// \name Sending Messages
  ///@{

  /// \brief Sends a reliable message without any data.
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezUInt32 uiSystemID, ezUInt32 uiMsgID);

  /// \brief Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezNetworkTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data);

  /// \brief Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezNetworkTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData = nullptr, ezUInt32 uiDataBytes = 0);

  /// \brief Sends an ezNetworkMessage
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezNetworkTransmitMode tm, ezNetworkMessage& msg);

  ///@}

  /// \name Message Handling
  ///@{

  /// \brief Registers a message handler that is executed for all incoming messages for the given system
  void SetMessageHandler(ezUInt32 uiSystemID, ezNetworkMessageHandler messageHandler);

  /// \brief Executes the message handler for all messages that have arrived for the given system
  ezUInt32 ExecuteMessageHandlers(ezUInt32 uiSystem);

  /// \brief Executes all message handlers for all received messages
  ezUInt32 ExecuteAllMessageHandlers();

  ///@}

  /// \name Events
  ///@{

  /// \brief Broadcasts events about connections
  ezEvent<const ezNetworkEvent&> m_NetworkEvents;

  ///@}

protected:

  /// \name Implementation Details
  ///@{

  /// \brief Derived classes have to implement this to start a network connection
  virtual ezResult InternalCreateConnection(ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress) = 0;

  /// \brief Derived classes have to implement this to shutdown a network connection
  virtual void InternalShutdownConnection() = 0;

  /// \brief Derived classes have to implement this to update the network
  virtual void InternalUpdateNetwork() = 0;

  /// \brief Derived classes have to implement this to get the ping to the server (client mode only)
  virtual ezTime InternalGetPingToServer() = 0;

  /// \brief Derived classes have to implement this to deliver messages to the server or client
  virtual ezResult InternalTransmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) = 0;

  /// \brief Derived classes can override this to interpret an address differently
  virtual ezResult DetermineTargetAddress(const char* szConnectTo, ezUInt32& out_IP, ezUInt16& out_Port);

  /// Derived classes should update this when the information is available
  ezString m_ServerInfoName;
  /// Derived classes should update this when the information is available
  ezString m_ServerInfoIP;

  /// \brief Should be called by the implementation, when a server connection has been established
  void ReportConnectionToServer(ezUInt32 uiServerID);
  /// \brief Should be called by the implementation, when a client connection has been established
  void ReportConnectionToClient(ezUInt32 uiApplicationID);
  /// \brief Should be called by the implementation, when a server connection has been lost
  void ReportDisconnectedFromServer();
  /// \brief Should be called by the implementation, when a client connection has been lost
  void ReportDisconnectedFromClient(ezUInt32 uiApplicationID);
  /// \brief Should be called by the implementation, when a message has arrived
  void ReportMessage(ezUInt32 uiApplicationID, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data);

  ///@}


private:
  void StartUpdateThread();
  void StopUpdateThread();
  ezResult Transmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data);
  ezResult CreateConnection(ezUInt32 uiConnectionToken, ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress, bool bStartUpdateThread);
  ezUInt32 ExecuteMessageHandlersForQueue(ezNetworkMessageQueue& queue);

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

/// \brief The network thread updates the given network in regular intervals to keep the connection alive.
///
/// The thread does NOT call ezNetworkInterface::ExecuteAllMessageHandlers(), so by default no message handlers are executed.
/// This has to be done manually by the application elsewhere.
class EZ_FILESERVEPLUGIN_DLL ezNetworkThread : public ezThread
{
public:
  ezNetworkThread();

  ezNetworkInterface* m_pNetwork = nullptr;
  volatile bool m_bKeepRunning = true;

private:
  virtual ezUInt32 Run();
};



