#include <PCH.h>
#include <EditorEngineView/Application.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_APPLICATION_ENTRY_POINT(ezEditorEngineViewApp);

ezEditorEngineViewApp::ezEditorEngineViewApp()
{
  EnableMemoryLeakReporting(true);
  m_pApp = nullptr;
}

void ezEditorEngineViewApp::AfterEngineInit()
{
  int argc = GetArgumentCount();
  const char** argv = GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**) argv);

  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezDelegate<void(const ezProcessCommunication::Event&)>(&ezEditorEngineViewApp::EventHandlerIPC, this));

  InitDevice();
}

void ezEditorEngineViewApp::BeforeEngineShutdown()
{
  m_IPC.m_Events.RemoveEventHandler(ezDelegate<void(const ezProcessCommunication::Event&)>(&ezEditorEngineViewApp::EventHandlerIPC, this));

  delete m_pApp;
}

ezApplication::ApplicationExecution ezEditorEngineViewApp::Run()
{
  m_IPC.ProcessMessages();

  ezThreadUtils::Sleep(1);
  return ezApplication::Continue;
}



