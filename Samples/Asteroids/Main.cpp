#include <ThirdParty/enet/enet.h>
#include "Main.h"
#include "Application.h"
#include "Window.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>

#include <RendererGL/Device/DeviceGL.h>

SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_pWindow = nullptr;
  m_pDevice = NULL;
}

void SampleGameApp::AfterEngineInit()
{
  EZ_LOG_BLOCK("SampleGameApp::AfterEngineInit");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory());

  ezTelemetry::CreateServer();

  if (ezPlugin::LoadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

  // Setup the logging system
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);

  InitRendering();

  ezStartup::StartupEngine();

  ezClock::SetNumGlobalClocks();
  ezClock::Get()->SetTimeStepSmoothing(&m_TimeStepSmoother);

  // Map the input keys to actions
  SetupInput();

  srand((ezUInt32) ezTime::Now().GetMicroseconds());

  CreateGameLevel();

  

  m_bActiveRenderLoop = true;
}

void SampleGameApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  ezStartup::ShutdownEngine();

  m_pDevice->Shutdown();
  EZ_DEFAULT_DELETE(m_pDevice);

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);

  DestroyGameLevel();

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_pWindow->ProcessWindowMessages();

  if (!m_bActiveRenderLoop)
    return ezApplication::Quit;

  ezClock::UpdateAllGlobalClocks();

  RenderSingleFrame();
  

  ezTelemetry::PerFrameUpdate();

  return ezApplication::Continue;
}


EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





