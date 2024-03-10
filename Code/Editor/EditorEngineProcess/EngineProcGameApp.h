#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Logging/HTMLWriter.h>
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
  using SUPER = ezGameApplication;

  ezEngineProcessGameApplication();
  ~ezEngineProcessGameApplication();

  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual ezApplication::Execution Run() override;

  void LogWriter(const ezLoggingEventData& e);

protected:
  virtual void BaseInit_ConfigureLogging() override;
  virtual void Deinit_ShutdownLogging() override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual bool Run_ProcessApplicationInput() override;
  virtual ezUniquePtr<ezEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  { /* do nothing */
  }

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();
  static bool EditorAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);
  void AddEditorAssertHandler();
  void RemoveEditorAssertHandler();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const ezEngineProcessCommunicationChannel::Event& e);
  void EventHandlerCVar(const ezCVarEvent& e);
  void EventHandlerCVarPlugin(const ezPluginEvent& e);
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
  ezLogWriter::HTML m_LogHTML;

  ezUInt32 m_uiRedrawCountReceived = 0;
  ezUInt32 m_uiRedrawCountExecuted = 0;
};
