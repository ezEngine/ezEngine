#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Communication/Event.h>

class QStringList;
class QProcess;
class ezIpcChannel;
class ezProcessMessage;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezProcessCommunicationChannel
{
public:
  ezProcessCommunicationChannel();
  ~ezProcessCommunicationChannel();

  ezResult StartClientProcess(const char* szProcess, const QStringList& args, const ezRTTI* pFirstAllowedMessageType = nullptr, ezUInt32 uiMemSize = 1024 * 1024 * 10);

  ezResult ConnectToHostProcess();

  void CloseConnection();

  bool IsClientAlive() const;

  bool IsHostAlive() const;

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

private:
  void MessageFunc(const ezProcessMessage* msg);

  ezIpcChannel* m_pChannel;
  WaitForMessageCallback m_WaitForMessageCallback;
  const ezRTTI* m_pWaitForMessageType;
  const ezRTTI* m_pFirstAllowedMessageType;
  ezInt64 m_iHostPID;
  ezUInt32 m_uiProcessID;
  QProcess* m_pClientProcess;
};

