#include "Main.h"
#include "Application.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_bFullscreen = false;
  m_uiResolutionX = 950;
  m_uiResolutionY = 950;
  m_szAppName = "ezSampleGame";
}

void SampleGameApp::AfterEngineInit()
{
  // Setup the logging system
  ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Map the input keys to actions
  SetupInput();

  CreateGameLevel();
}

void SampleGameApp::BeforeEngineShutdown()
{
  DestroyGameLevel();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  CreateAppWindow();
  GameLoop();
  DestroyAppWindow();

  return ezApplication::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





