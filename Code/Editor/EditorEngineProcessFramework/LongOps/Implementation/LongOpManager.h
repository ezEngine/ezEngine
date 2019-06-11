#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpManager
{
public:
  void Startup(ezProcessCommunicationChannel* pCommunicationChannel);
  void Shutdown();

  mutable ezMutex m_Mutex;

protected:
  virtual void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e) = 0;

  ezProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  ezEvent<const ezProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
