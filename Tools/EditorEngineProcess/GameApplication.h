#include <PCH.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GameFoundation/GameApplication.h>

class ezEngineProcessGameApplication : public ezGameApplication
{
public:
  ezEngineProcessGameApplication();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

  void LogWriter(const ezLoggingEventData & e);

private:
  void ProcessIPCMessages();
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  ezEngineProcessDocumentContext* CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg);

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
  ezGameStateBase m_GameState;
};

