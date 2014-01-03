#include <ThirdParty/enet/enet.h>
#include "Main.h"
#include "Application.h"
#include "Window.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>



SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_pWindow = NULL;
}

void SampleGameApp::AfterEngineInit()
{
  EZ_LOG_BLOCK("SampleGameApp::AfterEngineInit");

  ezTelemetry::CreateServer();

  if (ezPlugin::LoadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

  // Setup the logging system
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);

  ezStartup::StartupEngine();

  ezClock::SetNumGlobalClocks();
  ezClock::Get()->SetTimeStepSmoothing(&m_TimeStepSmoother);

  // Map the input keys to actions
  SetupInput();

  srand((ezUInt32) ezSystemTime::Now().GetMicroseconds());

  CreateGameLevel();

  

  m_bActiveRenderLoop = true;
}

void SampleGameApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  ezStartup::ShutdownEngine();

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);

  DestroyGameLevel();

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_bActiveRenderLoop = m_bActiveRenderLoop && (m_pWindow->ProcessWindowMessages() == ezWindow::Continue);

  if(!m_bActiveRenderLoop)
    return ezApplication::Quit;

  ezClock::UpdateAllGlobalClocks();

  RenderSingleFrame();
  m_pWindow->PresentFrame();
  

  ezTelemetry::PerFrameUpdate();

  return ezApplication::Continue;
}


EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





