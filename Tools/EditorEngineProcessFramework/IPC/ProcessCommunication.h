#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Reflection/Reflection.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

#if 0

class QProcess;
class QSharedMemory;
class QStringList;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezProcessCommunicationQt
{
public:
  ezProcessCommunicationQt();

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

  bool ProcessMessages(bool bAllowMsgDispatch = true);
  void WaitForMessages();

  struct Event
  {
    const ezProcessMessage* m_pMessage;
  };

  ezEvent<const Event&> m_Events;

private:
  bool ReadMessages();
  void WriteMessages();
  void DispatchMessages();


  ezMutex m_SendQueueMutex;
  WaitForMessageCallback m_WaitForMessageCallback;
  const ezRTTI* m_pWaitForMessageType;
  const ezRTTI* m_pFirstAllowedMessageType;
  ezInt64 m_iHostPID;
  ezUInt32 m_uiProcessID;
  QProcess* m_pClientProcess;
  QSharedMemory* m_pSharedMemory;
  ezDeque<ezMemoryStreamStorage> m_MessageSendQueue;
  ezDeque<ezMemoryStreamStorage> m_MessageReadQueue;
};

#endif

typedef ezProcessCommunicationChannel ezProcessCommunication;
