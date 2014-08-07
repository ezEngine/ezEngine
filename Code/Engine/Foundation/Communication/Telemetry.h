#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Communication/Implementation/TelemetryMessage.h>
#include <Foundation/Threading/Mutex.h>


/// \todo document and test (and finish)
class EZ_FOUNDATION_DLL ezTelemetry
{
public:

  /// \brief The port over which ezTelemetry will connect.
  static ezUInt16 s_uiPort;

  /// \brief Defines how the ezTelemetry system was configured.
  enum ConnectionMode
  {
    None,     ///< Not configured yet, at all.
    Server,   ///< Set up as a Server, i.e. this is an application that broadcasts information about its current state to one or several Clients.
    Client,   ///< Set up as a Client, i.e. this is a tool that gathers information from a Server, usually for debugging/inspection use cases.
  };

  /// \brief Describes how to send messages.
  enum TransmitMode
  {
    Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
    Unreliable, ///< Messages are sent at most once, if they get lost, they are not resend. If it is known beforehand, that not receiver exists, they are dropped without sending them at all.
  };

  /// \name Connection Configuration
  /// @{ 

  /// \brief Starts a connection as a 'Client' to one server.
  ///
  /// \param szConnectTo String that contains a host name or IP address to connect to. If empty, 'localhost' is used.
  ///
  /// \note Connections to invalid host names often succeed, because the underlying network API will fall back to 'localhost'.
  /// Connections to invalid IP addresses will however always fail.
  ///
  /// This function will set the ezTelemetry connection mode to 'Client'. This is mutually exclusive with CreateServer().
  static ezResult ConnectToServer(const char* szConnectTo = nullptr);

  /// \brief Opens a connection as a server.
  ///
  /// Other applications can then connect to this application using ConnectToServer() with the IP of this machine.
  static void CreateServer();

  /// \brief Closes any connection previously opened using ConnectToServer() or CreateServer().
  ///
  /// This will remove all queued incoming and outgoing messages (though it might send some of them still).
  /// It will not reset the state of which messages are filtered out or which callbacks to fire.
  static void CloseConnection();


  /// @}

  /// \name Sending Data
  /// @{ 

  static void Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);
  static void Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReaderBase& Stream, ezInt32 iDataBytes = -1);
  static void Broadcast(TransmitMode tm, ezTelemetryMessage& Msg);

  static void SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData = nullptr, ezUInt32 uiDataBytes = 0);
  static void SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReaderBase& Stream, ezInt32 iDataBytes = -1);
  static void SendToServer(ezTelemetryMessage& Msg);

  /// @}

  /// \name Querying State
  /// @{ 

  /// \brief Returns whether the telemetry system is set up as Server, Client or not initialized at all.
  static ConnectionMode GetConnectionMode() { return s_ConnectionMode; }

  /// \brief Returns whether a Client has an active connection to a Server.
  static bool IsConnectedToServer() { return s_bConnectedToServer; }

  /// \brief Returns whether a Server has an active connection to at least one Client.
  static bool IsConnectedToClient() { return s_bConnectedToClient; }

  /// \brief Returns whether a connection to another application has been made. Does not differentiate between Server and Client mode.
  static bool IsConnectedToOther();

  /// \brief Returns the last round trip time ('Ping') to the Server. Only meaningful if there is an active connection (see IsConnectedToServer() ).
  static ezTime GetPingToServer() { return s_PingToServer; }

  /// \brief Returns the name of the machine on which the Server is running. Only meaningful if there is an active connection (see IsConnectedToServer() ).
  static const char* GetServerName() { return s_ServerName.GetData(); }

  /// \brief Returns the IP address of the machine on which the Server is running. Only meaningful if there is an active connection (see IsConnectedToServer() ).
  static const char* GetServerIP() { return s_ServerIP.GetData(); }

  /// \brief Returns a 'unique' ID for the application instance to which this Client is connected.
  ///
  /// Only meaningful if there is an active connection (see IsConnectedToServer() ).
  /// This can be used when a connection got lost and a Client had to reconnect to the Server, to check whether the instance that the Client connected
  /// to is still the same as before. If it did not change, the application can simply continue gathering data. Otherwise it should clear its state
  /// and start from scratch.
  static ezUInt32 GetServerID() { return s_uiServerID; }

  /// \brief Returns the internal mutex used to synchronize all telemetry data access.
  ///
  /// This can be used to block all threads from accessing telemetry data, thus stopping the application.
  /// This can be useful when you want to implement some operation that is fully synchronous with some external tool and you want to
  /// wait for its response and prevent all other actions while you wait for that.
  static ezMutex& GetTelemetryMutex();

  /// @}

  /// \name Processing Messages
  /// @{ 

  /// \brief Checks whether any message for the system with the given ID exists and returns that.
  ///
  /// If no message for the given system is available, EZ_FAILURE is returned.
  /// This function will not poll the network to check whether new messages arrived.
  /// Use UpdateNetwork() and RetrieveMessage() in a loop, if you are waiting for a specific message, 
  /// to continuously update the network state and check whether the desired message has arrived.
  /// However, if you do so, you will be able to deadlock your application, if such a message never arrives.
  /// Also it might fill up other message queues which might lead to messages getting discarded.
  static ezResult RetrieveMessage(ezUInt32 uiSystemID, ezTelemetryMessage& out_Message);

  /// \brief Polls the network for new incoming messages and ensures outgoing messages are sent.
  ///
  /// Usually it is not necessary to call this function manually, as a worker thread will do that periodically already.
  /// However, if you are waiting for a specific message (see RetrieveMessage() ), you can call this function in a loop
  /// together with RetrieveMessage() to wait for that message.
  /// In that case it might also make sense to use GetTelemetryMutex() to lock the entire section while waiting for the message.
  static void UpdateNetwork();

  typedef void(*ProcessMessagesCallback)(void* pPassThrough);

  static void AcceptMessagesForSystem(ezUInt32 uiSystemID, bool bAccept, ProcessMessagesCallback Callback = nullptr, void* pPassThrough = nullptr);

  /// \brief Call this once per frame to process queued messages and to send the PerFrameUpdate event.
  static void PerFrameUpdate();

  /// \brief Specifies how many reliable messages from a system might get queued when no recipient is available yet.
  ///
  /// \param uiSystemID The ID for the system that sends the messages.
  /// \param uiMaxQueued The maximum number of reliable messages that get queued and delivered later, once
  ///        a proper recipient is available. Set this to zero to discard all messages from a system, when no recipient is available.
  ///
  /// The default queue size is 1000. When a connection to a suitable recipient is made, all queued messages are delivered in one burst.
  static void SetOutgoingQueueSize(ezUInt32 uiSystemID, ezUInt16 uiMaxQueued);

  /// @}

  /// \name ezTelemetry Events
  /// @{ 

  struct TelemetryEventData
  {
    enum EventType
    {
      ConnectedToClient,        ///< brief Send whenever a new connection to a client has been established.
      ConnectedToServer,        ///< brief Send whenever a connection to the server has been established.
      DisconnectedFromClient,   ///< Send every time the connection to a client is dropped
      DisconnectedFromServer,   ///< Send when the connection to the server has been lost
      PerFrameUpdate,           ///< Send once per frame, react to this to send per-frame statistics
    };

    EventType m_EventType;
  };

  typedef ezEvent<const TelemetryEventData&, ezMutex> ezEventTelemetry;

  /// \brief Adds an event handler that is called for every ezTelemetry event.
  static void AddEventHandler(ezEventTelemetry::Handler handler)    { s_TelemetryEvents.AddEventHandler    (handler); }

  /// \brief Removes a previously added event handler.
  static void RemoveEventHandler(ezEventTelemetry::Handler handler) { s_TelemetryEvents.RemoveEventHandler (handler); }

  /// @}

private:
  static void UpdateServerPing();

  static ezResult OpenConnection(ConnectionMode Mode, const char* szConnectTo = nullptr);

  static void Transmit(TransmitMode tm, const void* pData, ezUInt32 uiDataBytes);

  static void Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);
  static void Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezStreamReaderBase& Stream, ezInt32 iDataBytes = -1);
  static void Send(TransmitMode tm, ezTelemetryMessage& msg);

  friend class ezTelemetryThread;

  static void FlushOutgoingQueues();

  static void InitializeAsServer();
  static ezResult InitializeAsClient(const char* szConnectTo);
  static ConnectionMode s_ConnectionMode;

  static ezUInt32 s_uiApplicationID;
  static ezUInt32 s_uiServerID;

  static ezString s_ServerName;
  static ezString s_ServerIP;

  static bool s_bConnectedToServer;
  static bool s_bConnectedToClient;
  static bool s_bAllowNetworkUpdate;

  static void QueueOutgoingMessage(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);

  static ezTime s_PingToServer;

  typedef ezDeque<ezTelemetryMessage> MessageDeque;

  struct MessageQueue
  {
    MessageQueue() 
    { 
      m_bAcceptMessages = false; 
      m_uiMaxQueuedOutgoing = 1000; 
      m_Callback = nullptr;
      m_pPassThrough = nullptr;
    }

    bool m_bAcceptMessages;
    ProcessMessagesCallback m_Callback;
    void* m_pPassThrough;
    ezUInt32 m_uiMaxQueuedOutgoing;

    MessageDeque m_IncomingQueue;
    MessageDeque m_OutgoingQueue;
  };

  static ezMap<ezUInt64, MessageQueue> s_SystemMessages;
  
  static ezEventTelemetry s_TelemetryEvents;

private:
  static ezMutex s_TelemetryMutex;
  static void StartTelemetryThread();
  static void StopTelemetryThread();
};

