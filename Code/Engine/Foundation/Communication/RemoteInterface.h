#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Communication/Event.h>

/// \brief Whether the remote interface is configured as a server or a client
enum class ezRemoteMode
{
  None,   ///< Remote interface is shut down
  Server, ///< Remote interface acts as a server. Can connect with multiple clients
  Client  ///< Remote interface acts as a client. Can connect with exactly one server.
};

/// \brief Mode for transmitting messages
///
/// Depending on the remote interface implementation, Unreliable may not be supported and revert to Reliable.
enum class ezRemoteTransmitMode
{
  Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
  Unreliable, ///< Messages are sent at most once, if they get lost, they are not resent. If it is known beforehand, that not receiver exists, they are dropped without sending them at all.
};

/// \brief Event type for connections
struct EZ_FOUNDATION_DLL ezRemoteEvent
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

typedef ezDelegate<void(ezRemoteMessage&)> ezRemoteMessageHandler;

struct EZ_FOUNDATION_DLL ezRemoteMessageQueue
{
  ezRemoteMessageHandler m_MessageHandler;
  ezDeque<ezRemoteMessage> m_MessageQueue;
};

class EZ_FOUNDATION_DLL ezRemoteInterface
{
public:
  virtual ~ezRemoteInterface();

  /// \brief Exposes the mutex that is internally used to secure multi-threaded access
  ezMutex& GetMutex() const { return m_Mutex; }


  /// \name Connection
  ///@{

  /// \brief Starts the remote interface as a server.
  ///
  /// \param uiConnectionToken Should be a unique sequence (e.g. 'EZPZ') to identify the purpose of this connection.
  /// Only server and clients with the same token will accept connections.
  /// \param uiPort The port over which the connection should run.
  /// \param bStartUpdateThread If true, a thread is started that will regularly call UpdateNetwork() and UpdatePingToServer().
  /// If false, this has to be called manually in regular intervals.
  ezResult StartServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread = true);

  /// \brief Starts the network interface as a client. Tries to connect to the given address.
  ///
  /// This function immediately returns and no connection is guaranteed.
  /// \param uiConnectionToken Same as for StartServer()
  /// \param szAddress Could be a network address "127.0.0.1" or "localhost" or some other name that identifies the target, e.g. a named pipe.
  /// \param bStartUpdateThread Same as for StartServer()
  ///
  /// If this function succeeds, it still might not be connected to a server.
  /// Use WaitForConnectionToServer() to enforce a connection.
  ezResult ConnectToServer(ezUInt32 uiConnectionToken, const char* szAddress, bool bStartUpdateThread = true);

  /// \brief Can only be called after ConnectToServer(). Updates the network in a loop until a connection is established, or the time has run out.
  ///
  /// A timeout of exactly zero means to wait indefinitely.
  ezResult WaitForConnectionToServer(ezTime timeout = ezTime::Seconds(10));

  /// \brief Closes the connection in an orderly fashion
  void ShutdownConnection();

  /// \brief Whether the client is connected to a server
  bool IsConnectedToServer() const { return m_uiConnectedToServerWithID != 0; }

  /// \brief Whether the server is connected to any client
  bool IsConnectedToClients() const { return m_iConnectionsToClients > 0; }

  /// \brief Whether the client or server is connected its counterpart
  bool IsConnectedToOther() const { return IsConnectedToServer() || IsConnectedToClients(); }

  /// \brief Whether the remote interface is inactive, a client or a server
  ezRemoteMode GetRemoteMode() const { return m_RemoteMode; }

  /// \brief The address through which the connection was started
  const ezString& GetServerAddress() const { return m_sServerAddress; }

  /// \brief Returns the own (random) application ID used to identify this instance
  ezUInt32 GetApplicationID() const { return m_uiApplicationID; }

  /// \brief Returns the connection token used to identify compatible servers/clients
  ezUInt32 GetConnectionToken() const { return m_uiConnectionToken; }

  ///@}

  /// \name Server Information
  ///@{

  /// \brief For the client to display the name of the server
  //const ezString& GetServerInfoName() const { return m_ServerInfoName; }

  /// \brief For the client to display the IP of the server
  const ezString& GetServerInfoIP() const { return m_ServerInfoIP; }

  /// \brief Some random identifier, that allows to determine after a reconnect, whether the connected instance is still the same server
  ezUInt32 GetServerID() const { return m_uiConnectedToServerWithID; }

  /// \brief Returns the current ping to the server
  ezTime GetPingToServer() const { return m_PingToServer; }

  ///@}

  /// \name Updating the Remote Interface
  ///@{

  /// \brief If no update thread was spawned, this should be called to process messages
  void UpdateRemoteInterface();

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
  void Send(ezRemoteTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const ezArrayPtr<const ezUInt8>& data);

  /// \brief Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezRemoteTransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData = nullptr, ezUInt32 uiDataBytes = 0);

  /// \brief Sends an ezRemoteMessage
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(ezRemoteTransmitMode tm, ezRemoteMessage& msg);

  ///@}

  /// \name Message Handling
  ///@{

  /// \brief Registers a message handler that is executed for all incoming messages for the given system
  void SetMessageHandler(ezUInt32 uiSystemID, ezRemoteMessageHandler messageHandler);

  /// \brief Executes the message handler for all messages that have arrived for the given system
  ezUInt32 ExecuteMessageHandlers(ezUInt32 uiSystem);

  /// \brief Executes all message handlers for all received messages
  ezUInt32 ExecuteAllMessageHandlers();

  ///@}

  /// \name Events
  ///@{

  /// \brief Broadcasts events about connections
  ezEvent<const ezRemoteEvent&> m_RemoteEvents;

  ///@}

protected:

  /// \name Implementation Details
  ///@{

  /// \brief Derived classes have to implement this to start a network connection
  virtual ezResult InternalCreateConnection(ezRemoteMode mode, const char* szServerAddress) = 0;

  /// \brief Derived classes have to implement this to shutdown a network connection
  virtual void InternalShutdownConnection() = 0;

  /// \brief Derived classes have to implement this to update
  virtual void InternalUpdateRemoteInterface() = 0;

  /// \brief Derived classes have to implement this to get the ping to the server (client mode only)
  virtual ezTime InternalGetPingToServer() = 0;

  /// \brief Derived classes have to implement this to deliver messages to the server or client
  virtual ezResult InternalTransmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) = 0;

  /// \brief Derived classes can override this to interpret an address differently
  virtual ezResult DetermineTargetAddress(const char* szConnectTo, ezUInt32& out_IP, ezUInt16& out_Port);

  /// Derived classes should update this when the information is available
  //ezString m_ServerInfoName;
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
  ezResult Transmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data);
  ezResult CreateConnection(ezUInt32 uiConnectionToken, ezRemoteMode mode, const char* szServerAddress, bool bStartUpdateThread);
  ezUInt32 ExecuteMessageHandlersForQueue(ezRemoteMessageQueue& queue);

  mutable ezMutex m_Mutex;
  class ezRemoteThread* m_pUpdateThread = nullptr;
  ezRemoteMode m_RemoteMode = ezRemoteMode::None;
  ezString m_sServerAddress;
  ezTime m_PingToServer;
  ezUInt32 m_uiApplicationID = 0; // sent when connecting to identify the sending instance
  ezUInt32 m_uiConnectionToken = 0;
  ezUInt32 m_uiConnectedToServerWithID = 0;
  ezInt32 m_iConnectionsToClients = 0;
  ezDynamicArray<ezUInt8> m_TempSendBuffer;
  ezHashTable<ezUInt32, ezRemoteMessageQueue> m_MessageQueues;
};

/// \brief The remote interface thread updates in regular intervals to keep the connection alive.
///
/// The thread does NOT call ezRemoteInterface::ExecuteAllMessageHandlers(), so by default no message handlers are executed.
/// This has to be done manually by the application elsewhere.
class EZ_FOUNDATION_DLL ezRemoteThread : public ezThread
{
public:
  ezRemoteThread();

  ezRemoteInterface* m_pRemoteInterface = nullptr;
  volatile bool m_bKeepRunning = true;

private:
  virtual ezUInt32 Run();
};


