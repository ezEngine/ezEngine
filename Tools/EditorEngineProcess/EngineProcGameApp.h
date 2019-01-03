#pragma once

#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>

class ezEditorEngineProcessApp;
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

  void LogWriter(const ezLoggingEventData& e);

protected:
  virtual void DoSetupLogWriters() override;
  virtual void DoShutdownLogWriters() override;
  virtual void DoSetupDataDirectories() override;
  virtual void ProcessApplicationInput() override;
  virtual ezUniquePtr<ezEditorEngineProcessApp> CreateEngineProcessApp();


  virtual bool GetActivateGameStateAtStartup() const override { return false; }

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const ezEngineProcessCommunicationChannel::Event& e);
  void EventHandlerTypeUpdated(const ezRTTI* pType);
  void EventHandlerCVar(const ezCVar::CVarEvent& e);
  void EventHandlerCVarPlugin(const ezPlugin::PluginEvent& e);
  void TransmitCVar(const ezCVar* pCVar);

  ezEngineProcessDocumentContext* CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg);

  virtual void DoLoadPluginsFromConfig() override;

  virtual ezString FindProjectDirectory() const override;

  ezString m_sProjectDirectory;
  ezApplicationFileSystemConfig m_CustomFileSystemConfig;
  ezApplicationPluginConfig m_CustomPluginConfig;
  ezEngineProcessCommunicationChannel m_IPC;
  ezUniquePtr<ezEditorEngineProcessApp> m_pApp;
};
