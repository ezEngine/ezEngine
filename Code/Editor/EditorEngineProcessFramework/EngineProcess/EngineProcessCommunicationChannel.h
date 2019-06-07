#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineProcessCommunicationChannel : public ezProcessCommunicationChannel
{
public:
  ezResult ConnectToHostProcess();

  bool IsHostAlive() const;

private:
  ezInt64 m_iHostPID = 0;
};

