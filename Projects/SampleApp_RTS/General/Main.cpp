#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/General/Window.h>
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
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 90.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0, 10, 0), ezVec3(0, 0, -20));
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

  m_pRenderer = EZ_DEFAULT_NEW(GameRenderer);

  m_pLevel = EZ_DEFAULT_NEW(Level);
  m_pLevel->SetupLevel();

  m_pSelectedUnits = EZ_DEFAULT_NEW(ezObjectSelection);
  m_pSelectedUnits->SetWorld(m_pLevel->GetWorld());

  m_pRenderer->SetupRenderer(m_pWindow, m_pLevel, &m_Camera, &m_pLevel->GetNavmesh());

  m_bActiveRenderLoop = true;
}

void SampleGameApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  EZ_DEFAULT_DELETE(m_pSelectedUnits);
  EZ_DEFAULT_DELETE(m_pLevel);
  EZ_DEFAULT_DELETE(m_pRenderer);

  ezStartup::ShutdownEngine();

  EZ_DEFAULT_DELETE(m_pWindow);
  

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_bActiveRenderLoop = m_bActiveRenderLoop && (m_pWindow->ProcessWindowMessages() == ezWindow::Continue);

  if(!m_bActiveRenderLoop)
    return ezApplication::Quit;

  ezClock::UpdateAllGlobalClocks();

  {
    const ezTime tNow = ezSystemTime::Now();

    static ezTime s_LastGameUpdate = tNow;

    UpdateInput(tNow - s_LastGameUpdate);
    m_pLevel->Update();

    s_LastGameUpdate = tNow;

    m_pRenderer->RenderLevel(m_pSelectedUnits);
  }

  m_pWindow->PresentFrame();
  

  ezTelemetry::PerFrameUpdate();

  return ezApplication::Continue;
}



EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





