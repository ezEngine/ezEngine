#pragma once

#include <PCH.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GameEngine/GameApplication/GameApplication.h>

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
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  ezEngineProcessDocumentContext* CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg);

  virtual void DoLoadPluginsFromConfig() override;

  virtual ezString FindProjectDirectory() const override;

  ezString m_sProjectDirectory;
  ezApplicationFileSystemConfig m_CustomFileSystemConfig;
  ezApplicationPluginConfig m_CustomPluginConfig;
  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
};

