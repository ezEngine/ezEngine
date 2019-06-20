#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

/// \brief Base class with shared functionality for ezLongOpControllerManager and ezLongOpWorkerManager
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpManager
{
public:
  /// \brief Needs to be called early to initialize the IPC channel to use.
  void Startup(ezProcessCommunicationChannel* pCommunicationChannel);

  /// \brief Call this to shut down the IPC communication.
  void Shutdown();

  /// \brief Publicly exposed mutex for some special cases.
  mutable ezMutex m_Mutex;

protected:
  virtual void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e) = 0;

  ezProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  ezEvent<const ezProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
