#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class ezIpcChannel;
class ezMessageLoop;


/// \brief A protocol around ezIpcChannel to send reflected messages instead of byte array messages between client and server.
///
/// This wrapper class hooks into an existing ezIpcChannel. The ezIpcChannel is still responsible for all connection logic. This class merely provides a high-level messaging protocol via reflected messages derived from ezProcessMessage.
/// Note that if this class is used, ezIpcChannel::Send must not be called manually anymore, only use ezIpcProcessMessageProtocol::Send.
/// Received messages are stored in a queue and must be flushed via calling ProcessMessages or WaitForMessages.
class EZ_FOUNDATION_DLL ezIpcProcessMessageProtocol
{
public:
  ezIpcProcessMessageProtocol(ezIpcChannel* pChannel);
  ~ezIpcProcessMessageProtocol();

  /// \brief Sends a message. pMsg can be destroyed after the call.
  bool Send(ezProcessMessage* pMsg);


  /// \brief Processes all pending messages by broadcasting m_MessageEvent. Not re-entrant.
  bool ProcessMessages();
  /// \brief Block and wait for new messages and call ProcessMessages.
  ezResult WaitForMessages(ezTime timeout = ezTime::MakeZero());

public:
  struct Event
  {
    const ezProcessMessage* m_pMessage;
    // Set to true in a message handler to cancel the ProcessMessages function and return to the caller before all messages have been processed.
    mutable bool m_bInterruptMessageProcessing = false;
  };
  ezEvent<const Event&> m_MessageEvent; ///< Will be sent from thread calling ProcessMessages or WaitForMessages.

private:
  void EnqueueMessage(ezUniquePtr<ezProcessMessage>&& msg);
  ezUniquePtr<ezProcessMessage> PopMessage();
  void ReceiveMessageData(ezArrayPtr<const ezUInt8> data);

private:
  ezIpcChannel* m_pChannel = nullptr;

  ezMutex m_IncomingQueueMutex;
  ezDeque<ezUniquePtr<ezProcessMessage>> m_IncomingQueue;
};
