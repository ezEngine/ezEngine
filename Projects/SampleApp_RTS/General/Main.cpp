#include <PCH.h>
#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/General/Window.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/IO/JSONWriter.h>
#include <gl/GL.h>

SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_pWindow = NULL;
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 90.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0, 10, 0), ezVec3(0, 0, -20));
}

void SampleGameApp::AfterEngineInit()
{
  ShowWindow(GetConsoleWindow(), SW_HIDE);

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

  m_ScreenshotTransfer.EnableDataTransfer("Screenshot");
  m_StatsTransfer.EnableDataTransfer("Stats");
}

void SampleGameApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  if (ezPlugin::UnloadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

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
    const ezTime tNow = ezTime::Now();

    static ezTime s_LastGameUpdate = tNow;

    UpdateInput(tNow - s_LastGameUpdate);
    m_pLevel->Update();

    s_LastGameUpdate = tNow;

    if (!m_pWindow->IsMinimized())
    {
      m_pRenderer->RenderLevel(m_pSelectedUnits);
      m_pWindow->PresentFrame();
    }

    ezTaskSystem::FinishFrameTasks();
  }

  ezTelemetry::PerFrameUpdate();

  if (m_ScreenshotTransfer.IsTransferRequested())
  {
    

    ezUInt32 uiWidth = m_pWindow->GetResolution().width;
    ezUInt32 uiHeight = m_pWindow->GetResolution().height;

    ezDynamicArray<ezUInt8> Image;
    Image.SetCount(uiWidth * uiHeight * 4);

    glReadPixels(0, 0, uiWidth, uiHeight, GL_RGBA, GL_UNSIGNED_BYTE, &Image[0]);

    ezDynamicArray<ezUInt8> ImageCorrect;
    ImageCorrect.SetCount(Image.GetCount());

    for (ezUInt32 h = 0; h < uiHeight; ++h)
    {
      for (ezUInt32 w = 0; w < uiWidth; ++w)
      {
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 0] = Image[(h * uiWidth + w) * 4 + 2];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 1] = Image[(h * uiWidth + w) * 4 + 1];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 2] = Image[(h * uiWidth + w) * 4 + 0];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 3] = 255;
      }
    }

    ezDataTransferObject dto(m_ScreenshotTransfer, "Result", "image/rgba8");
    dto.GetWriter() << uiWidth;
    dto.GetWriter() << uiHeight;
    dto.GetWriter().WriteBytes(&ImageCorrect[0], ImageCorrect.GetCount());
    m_ScreenshotTransfer.Transfer(dto);

    ezDataTransferObject dto2(m_ScreenshotTransfer, "Result_Original", "image/rgba8");
    dto2.GetWriter() << uiWidth;
    dto2.GetWriter() << uiHeight;
    dto2.GetWriter().WriteBytes(&Image[0], Image.GetCount());
    m_ScreenshotTransfer.Transfer(dto2);
  }

  if (m_StatsTransfer.IsTransferRequested())
  {
    ezDataTransferObject dto(m_StatsTransfer, "Result", "text/xml");

    ezStandardJSONWriter jw;
    jw.SetOutputStream(&dto.GetWriter());
      jw.BeginObject();
    jw.AddVariableString("SomeString", "absolutely");
    jw.EndObject();

    m_StatsTransfer.Transfer(dto);
  }

  return ezApplication::Continue;
}



EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





