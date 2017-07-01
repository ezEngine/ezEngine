#pragma once

#include <PCH.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>

class ezDocumentOpenMsgToEngine;
class ezEngineProcessDocumentContext;

class ezEngineProcessGameApplication : public ezGameApplication
{
public:
  ezEngineProcessGameApplication();

  virtual void BeforeCoreStartup() override;
  virtual void AfterCoreStartup() override;

  virtual void BeforeCoreShutdown() override;
  virtual void AfterCoreShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

  void LogWriter(const ezLoggingEventData & e);

protected:
  virtual void DoSetupLogWriters() override;
  virtual void DoShutdownLogWriters() override;
  virtual void DoSetupDataDirectories() override;
  virtual void ProcessApplicationInput() override;

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const ezEngineProcessCommunicationChannel::Event& e);
  void EventHandlerTypeUpdated(const ezRTTI* pType);

  ezEngineProcessDocumentContext* CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg);

  virtual void DoLoadPluginsFromConfig() override;

  virtual ezString FindProjectDirectory() const override;

  ezString m_sProjectDirectory;
  ezApplicationFileSystemConfig m_CustomFileSystemConfig;
  ezApplicationPluginConfig m_CustomPluginConfig;
  ezEngineProcessCommunicationChannel m_IPC;
};

