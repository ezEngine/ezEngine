#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Communication/Event.h>

class ezIpcChannel;
class ezProcessMessage;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezProcessCommunicationChannel
{
public:
  ezProcessCommunicationChannel();
  ~ezProcessCommunicationChannel();

  void SendMessage(ezProcessMessage* pMessage);

  /// /brief Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  ///        the waiting ends. If false is returned the wait for the message continues.
  typedef ezDelegate<bool(ezProcessMessage*)> WaitForMessageCallback;
  ezResult WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout, WaitForMessageCallback* pMessageCallack = nullptr );

  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const ezProcessMessage* m_pMessage;
  };

  ezEvent<const Event&> m_Events;

  void MessageFunc(const ezProcessMessage* msg);

protected:

  ezIpcChannel* m_pChannel = nullptr;
  const ezRTTI* m_pFirstAllowedMessageType = nullptr;

private:

  WaitForMessageCallback m_WaitForMessageCallback;
  const ezRTTI* m_pWaitForMessageType = nullptr;
};
