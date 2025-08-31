#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class ezIpcChannel;
class ezProcessMessage;
class ezIpcProcessMessageProtocol;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezProcessCommunicationChannel
{
public:
  ezProcessCommunicationChannel();
  ~ezProcessCommunicationChannel();

  bool SendMessage(ezProcessMessage* pMessage);

  /// /brief Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  ///        the waiting ends. If false is returned the wait for the message continues.
  using WaitForMessageCallback = ezDelegate<bool(ezProcessMessage*)>;
  ezResult WaitForMessage(const ezRTTI* pMessageType, ezTime timeout, WaitForMessageCallback* pMessageCallack = nullptr);
  ezResult WaitForConnection(ezTime timeout);
  bool IsConnected() const;

  /// \brief Returns true if any message was processed
  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const ezProcessMessage* m_pMessage;
    // Set to true in a message handler to cancel the ProcessMessages function and return to the caller before all messages have been processed.
    mutable bool m_bInterruptMessageProcessing = false;
  };

  ezEvent<const Event&> m_Events;

  void MessageFunc(const ezIpcProcessMessageProtocol::Event& msg);

protected:
  ezUniquePtr<ezIpcProcessMessageProtocol> m_pProtocol;
  ezUniquePtr<ezIpcChannel> m_pChannel;
  const ezRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const ezRTTI* m_pWaitForMessageType = nullptr;
};
