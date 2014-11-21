#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_APPLICATION_ENTRY_POINT(ezEditorProcessApp);

ezEditorProcessApp::ezEditorProcessApp()
{
  EnableMemoryLeakReporting(true);
  m_pApp = nullptr;
}

void ezEditorProcessApp::AfterEngineInit()
{
  ezTelemetry::CreateServer();

  ezPlugin::LoadPlugin("ezInspectorPlugin");
  ezPlugin::LoadPlugin("ezEnginePluginTest");

  int argc = GetArgumentCount();
  const char** argv = GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**) argv);

  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezDelegate<void(const ezProcessCommunication::Event&)>(&ezEditorProcessApp::EventHandlerIPC, this));

  InitDevice();
}

void ezEditorProcessApp::BeforeEngineShutdown()
{
  ezTelemetry::CloseConnection();

  m_IPC.m_Events.RemoveEventHandler(ezDelegate<void(const ezProcessCommunication::Event&)>(&ezEditorProcessApp::EventHandlerIPC, this));

  delete m_pApp;
}

ezApplication::ApplicationExecution ezEditorProcessApp::Run()
{
  m_IPC.ProcessMessages();

  ezTelemetry::PerFrameUpdate();

  ezThreadUtils::Sleep(1);
  return ezApplication::Continue;
}



