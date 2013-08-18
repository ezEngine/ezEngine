#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Communication/Implementation/TelemetryMessage.h>
#include <Foundation/Threading/Mutex.h>


class EZ_FOUNDATION_DLL ezTelemetry
{
public:

  enum ConnectionMode
  {
    None,
    Server,
    Client,
  };

  enum TransmitMode
  {
    Reliable,
    Unreliable,
  };

  static ezResult ConnectToServer(const char* szConnectTo = NULL);
  static ezResult CreateServer();

  static void CloseConnection();

  static void Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);
  static void Broadcast(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes = -1);
  static void Broadcast(TransmitMode tm, ezTelemetryMessage& Msg);

  static void SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);
  static void SendToServer(ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes = -1);
  static void SendToServer(ezTelemetryMessage& Msg);

  static ConnectionMode GetConnectionMode() { return s_ConnectionMode; }

  static bool IsConnectedToServer() { return s_bConnectedToServer; }
  static bool IsConnectedToClient() { return s_bConnectedToClient; }
  static ezTime GetPingToServer() { return s_PingToServer; }

  static const char* GetServerName() { return s_ServerName.GetData(); }
  static const char* GetServerIP() { return s_ServerIP.GetData(); }

  static ezResult RetrieveMessage(ezUInt32 uiSystemID, ezTelemetryMessage& out_Message);

  static ezUInt32 GetServerID() { return s_uiServerID; }

  static void UpdateNetwork();

  static void AcceptMessagesForSystem(ezUInt32 uiSystemID, bool bAccept);

  /// \brief Specifies how many reliable messages from a system might get queued when no receipient is available yet.
  ///
  /// \param uiSystemID The ID for the system that sends the messages.
  /// \param uiMaxQueued The maximum number of reliable messages that get queued and delivered later, once
  ///        a proper receipient is available. Set this to zero to discard all messages from a system, when no receipient is available.
  ///
  /// The default queue size is 1000. When a connection to a suitable receipient is made, all queued messages are delivered in one burst.
  static void SetOutgoingQueueSize(ezUInt32 uiSystemID, ezUInt16 uiMaxQueued);

  /// \brief Returns the internal mutex used to synchronize all telemetry data access.
  ///
  /// This can be used to block all threads from accessing telemetry data, thus stopping the application.
  /// This can be useful when you want to implement some operation that is fully synchronus with some external tool and you want to
  /// wait for its response and prevent all other actions while you wait for that.
  static ezMutex& GetTelemetryMutex();

private:
  static void UpdateServerPing();

  static ezResult OpenConnection(ConnectionMode Mode, const char* szConnectTo = NULL);

  static void Transmit(TransmitMode tm, const void* pData, ezUInt32 uiDataBytes);

  static void Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);
  static void Send(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, ezIBinaryStreamReader& Stream, ezInt32 iDataBytes = -1);
  static void Send(TransmitMode tm, ezTelemetryMessage& msg);

  friend class ezTelemetryThread;

  static bool IsConnectedToOther();

  static void FlushOutgoingQueues();

  static void InitializeAsServer();
  static ezResult InitializeAsClient(const char* szConnectTo);
  static ConnectionMode s_ConnectionMode;

  static ezUInt32 s_uiApplicationID;
  static ezUInt32 s_uiServerID;

  static ezHybridString<32, ezStaticAllocatorWrapper> s_ServerName;
  static ezHybridString<32, ezStaticAllocatorWrapper> s_ServerIP;

  static bool s_bConnectedToServer;
  static bool s_bConnectedToClient;

  static void QueueOutgoingMessage(TransmitMode tm, ezUInt32 uiSystemID, ezUInt32 uiMsgID, const void* pData, ezUInt32 uiDataBytes);

  static ezTime s_PingToServer;

  typedef ezDeque<ezTelemetryMessage, ezStaticAllocatorWrapper> MessageDeque;

  struct MessageQueue
  {
    MessageQueue() { m_bAcceptMessages = false; m_uiMaxQueuedOutgoing = 1000; }

    bool m_bAcceptMessages;
    ezUInt32 m_uiMaxQueuedOutgoing;

    MessageDeque m_IncomingQueue;
    MessageDeque m_OutgoingQueue;
  };

  static ezMap<ezUInt32, MessageQueue, ezCompareHelper<ezUInt32>, ezStaticAllocatorWrapper > s_SystemMessages;

private:
  static ezMutex s_TelemetryMutex;
  static void StartTelemetryThread();
  static void StopTelemetryThread();
};


namespace ezLogWriter
{
  class EZ_FOUNDATION_DLL NetworkBroadcast
  {
  public:

    static void LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough);

  };
}