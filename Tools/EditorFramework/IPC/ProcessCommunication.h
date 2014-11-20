#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/IO/MemoryStream.h>
#include <QProcess>
#include <QSharedMemory>

class EZ_EDITORFRAMEWORK_DLL ezProcessCommunication
{
public:
  ezProcessCommunication();

  ezResult StartClientProcess(const char* szProcess, ezUInt32 uiMemSize = 1024 * 1024 * 10);

  ezResult ConnectToHostProcess();

  void CloseConnection();

  bool IsClientAlive() const;

  void SendMessage(ezReflectedClass* pMessage);

  void ProcessMessages();

  struct Event
  {
    const ezReflectedClass* m_pMessage;
  };

  ezEvent<const Event&> m_Events;

private:
  bool ReadMessages();
  void WriteMessages();
  void DispatchMessages();

  ezUInt32 m_uiProcessID;
  QProcess* m_pClientProcess;
  QSharedMemory* m_pSharedMemory;
  ezDeque<ezMemoryStreamStorage> m_MessageSendQueue;
  ezDeque<ezMemoryStreamStorage> m_MessageReadQueue;
};


class EZ_EDITORFRAMEWORK_DLL ezEngineViewMsg : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineViewMsg);

public:

  ezUInt32 m_uiHWND;
  ezUInt16 m_uiWindowWidth;
  ezUInt16 m_uiWindowHeight;
};
