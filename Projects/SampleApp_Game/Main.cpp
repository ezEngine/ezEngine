#include <ThirdParty/enet/enet.h>
#include "Main.h"
#include "Application.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/TelemetryWriter.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>


SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_bFullscreen = false;
  m_uiResolutionX = 500;
  m_uiResolutionY = 500;
  m_szAppName = "ezSampleGame";
}

void SampleGameApp::AfterEngineInit()
{
  ezTelemetry::CreateServer();

  // Setup the logging system
  ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezLog::AddLogWriter(ezLogWriter::Telemetry::LogMessageHandler);

  // Map the input keys to actions
  SetupInput();

  srand((ezUInt32) ezSystemTime::Now().GetMicroSeconds());

  CreateGameLevel();

  CreateAppWindow();

  ezStartup::StartupEngine();

  
}

void SampleGameApp::BeforeEngineShutdown()
{
  ezStartup::ShutdownEngine();

  DestroyAppWindow();

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);

  DestroyGameLevel();

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  GameLoop();

  return ezApplication::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





