#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>

class ezEditorEngineProcessApp;
class ezDocumentOpenMsgToEngine;
class ezEngineProcessDocumentContext;
class ezResourceUpdateMsgToEngine;
class ezRestoreResourceMsgToEngine;

class ezEngineProcessGameApplication : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezEngineProcessGameApplication();

  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

  void LogWriter(const ezLoggingEventData& e);

protected:
  virtual void BaseInit_ConfigureLogging() override;
  virtual void Deinit_ShutdownLogging() override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual void Init_AddActorManagers() override;
  virtual bool Run_ProcessApplicationInput() override;
  virtual ezUniquePtr<ezEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  { /* do nothing */
  }



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

  void HandleResourceUpdateMsg(const ezResourceUpdateMsgToEngine& msg);
  void HandleResourceRestoreMsg(const ezRestoreResourceMsgToEngine& msg);

  ezEngineProcessDocumentContext* CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg);

  virtual void Init_LoadProjectPlugins() override;

  virtual ezString FindProjectDirectory() const override;

  ezString m_sProjectDirectory;
  ezApplicationFileSystemConfig m_CustomFileSystemConfig;
  ezApplicationPluginConfig m_CustomPluginConfig;
  ezEngineProcessCommunicationChannel m_IPC;
  ezUniquePtr<ezEditorEngineProcessApp> m_pApp;
  ezLongOpWorkerManager m_LongOpWorkerManager;
};
