#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class ezIpcChannel;
class ezMessageLoop;

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
  ezEvent<const ezProcessMessage*> m_MessageEvent; ///< Will be sent from thread calling ProcessMessages or WaitForMessages.

private:
  void EnqueueMessage(ezUniquePtr<ezProcessMessage>&& msg);
  void SwapWorkQueue(ezDeque<ezUniquePtr<ezProcessMessage>>& messages);
  void ReceiveMessageData(ezArrayPtr<const ezUInt8> data);

private:
  ezIpcChannel* m_pChannel = nullptr;

  ezMutex m_IncomingQueueMutex;
  ezDeque<ezUniquePtr<ezProcessMessage>> m_IncomingQueue;
};
