#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorFramework/EditorFrameworkDLL.h>

template <typename T>
class QList;
class QString;
using QStringList = QList<QString>;
class QProcess;

class EZ_EDITORFRAMEWORK_DLL ezEditorProcessCommunicationChannel : public ezProcessCommunicationChannel
{
public:
  ezResult StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const ezRTTI* pFirstAllowedMessageType = nullptr,
    ezUInt32 uiMemSize = 1024 * 1024 * 10);

  bool IsClientAlive() const;

  void CloseConnection();

  ezString GetStdoutContents();

private:
  QProcess* m_pClientProcess = nullptr;
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
