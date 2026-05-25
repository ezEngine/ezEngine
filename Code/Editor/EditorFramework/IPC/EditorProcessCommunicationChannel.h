#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Types/UniquePtr.h>

template <typename T>
class QList;
class QString;
using QStringList = QList<QString>;

class EZ_EDITORFRAMEWORK_DLL ezEditorProcessCommunicationChannel : public ezProcessCommunicationChannel
{
public:
  ezResult StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const ezRTTI* pFirstAllowedMessageType = nullptr,
    ezUInt32 uiMemSize = 1024 * 1024 * 10);
  bool IsClientAlive() const;
  void CloseConnection();
  ezString GetStdoutContents();
  ezOsProcessID GetProcessId() const;

private:
  ezUniquePtr<ezProcessGroup> m_pClientProcessGroup;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorProcessRemoteCommunicationChannel : public ezProcessCommunicationChannel
{
public:
  ezResult ConnectToServer(const char* szAddress);

  bool IsConnected() const;

  void CloseConnection();

  void TryConnect();

private:
};
