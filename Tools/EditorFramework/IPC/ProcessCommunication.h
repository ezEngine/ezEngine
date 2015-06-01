#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Reflection/Reflection.h>
#include <QProcess>
#include <QSharedMemory>

class EZ_EDITORFRAMEWORK_DLL ezProcessMessage : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessMessage);

public:

};

class EZ_EDITORFRAMEWORK_DLL ezProcessCommunication
{
public:
  ezProcessCommunication();

  ezResult StartClientProcess(const char* szProcess, const char* szArguments = nullptr, ezUInt32 uiMemSize = 1024 * 1024 * 10);

  ezResult ConnectToHostProcess();

  void CloseConnection();

  bool IsClientAlive() const;

  bool IsHostAlive() const;

  void SendMessage(ezProcessMessage* pMessage);

  void WaitForMessage(const ezRTTI* pMessageType);

  void ProcessMessages();

  struct Event
  {
    const ezProcessMessage* m_pMessage;
  };

  ezEvent<const Event&> m_Events;

private:
  bool ReadMessages();
  void WriteMessages();
  void DispatchMessages();

  const ezRTTI* m_pWaitForMessageType;
  ezInt64 m_iHostPID;
  ezUInt32 m_uiProcessID;
  QProcess* m_pClientProcess;
  QSharedMemory* m_pSharedMemory;
  ezDeque<ezMemoryStreamStorage> m_MessageSendQueue;
  ezDeque<ezMemoryStreamStorage> m_MessageReadQueue;
};

