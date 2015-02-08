#include <PCH.h>
#include <RTS/General/Application.h>
#include <RTS/General/Window.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/Formats/TgaFileFormat.h>
#include <gl/GL.h>

SampleGameApp::SampleGameApp()
{
  m_bActiveRenderLoop = false;
  m_pWindow = nullptr;
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 90.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0, 10, 0), ezVec3(0, 0, -20));
  m_bConsoleActive = false;
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

  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  sPath.AppendPath("../../../Shared/Samples/RTS");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_VERIFY(ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory()) == EZ_SUCCESS, "Failed to add data directory: '%s'", ezOSFile::GetApplicationDirectory());
  EZ_VERIFY(ezFileSystem::AddDataDirectory(sPath.GetData(), ezFileSystem::ReadOnly) == EZ_SUCCESS, "Failed to add data directory: '%s'", sPath.GetData());

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

  EZ_DEFAULT_DELETE(m_pSelectedUnits);
  EZ_DEFAULT_DELETE(m_pLevel);
  EZ_DEFAULT_DELETE(m_pRenderer);

  ezStartup::ShutdownEngine();

  EZ_DEFAULT_DELETE(m_pWindow);
  
  ezResourceManager::OnCoreShutdown();

  if (ezPlugin::UnloadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_pWindow->ProcessWindowMessages();

  if (!m_bActiveRenderLoop)
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
      m_pRenderer->RenderConsole(&m_Console, m_bConsoleActive);
      m_pRenderer->Present();
    }

    ezTaskSystem::FinishFrameTasks();
  }

  ezResourceManager::FreeUnusedResources(false);

  ezTelemetry::PerFrameUpdate();


  if (m_ScreenshotTransfer.IsTransferRequested())
  {
    ezUInt32 uiWidth = m_pWindow->GetResolution().width;
    ezUInt32 uiHeight = m_pWindow->GetResolution().height;

    ezDynamicArray<ezUInt8> Image;
    Image.SetCount(uiWidth * uiHeight * 4);

    glReadPixels(0, 0, uiWidth, uiHeight, GL_RGBA, GL_UNSIGNED_BYTE, &Image[0]);

    ezImage img;
    img.SetWidth(uiWidth);
    img.SetHeight(uiHeight);
    img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    img.AllocateImageData();

    ezDynamicArray<ezUInt8> ImageCorrect;
    ImageCorrect.SetCount(Image.GetCount());

    for (ezUInt32 h = 0; h < uiHeight; ++h)
    {
      for (ezUInt32 w = 0; w < uiWidth; ++w)
      {
        auto ptr = img.GetPixelPointer<ezUInt8>(0, 0, 0, w, uiHeight - h - 1, 0);
        ptr[0] = Image[(h * uiWidth + w) * 4 + 0];
        ptr[1] = Image[(h * uiWidth + w) * 4 + 1];
        ptr[2] = Image[(h * uiWidth + w) * 4 + 2];
        ptr[3] = Image[(h * uiWidth + w) * 4 + 3];

        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 0] = Image[(h * uiWidth + w) * 4 + 2];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 1] = Image[(h * uiWidth + w) * 4 + 1];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 2] = Image[(h * uiWidth + w) * 4 + 0];
        ImageCorrect[((uiHeight - h - 1) * uiWidth + w) * 4 + 3] = 255;
      }
    }

    ezDataTransferObject dto(m_ScreenshotTransfer, "Result", "image/rgba8", "rgba");
    dto.GetWriter() << uiWidth;
    dto.GetWriter() << uiHeight;
    dto.GetWriter().WriteBytes(&ImageCorrect[0], ImageCorrect.GetCount());
    dto.Transmit();

    ezDataTransferObject dto3(m_ScreenshotTransfer, "ResultTGA", "image/x-tga", "tga");

    ezTgaFileFormat tga;
    tga.WriteImage(dto3.GetWriter(), img, ezGlobalLog::GetInstance());

    dto3.Transmit();
  }

  if (m_StatsTransfer.IsTransferRequested())
  {
    ezDataTransferObject dto(m_StatsTransfer, "Result", "text/xml", "xml");

    ezStandardJSONWriter jw;
    jw.SetOutputStream(&dto.GetWriter());
      jw.BeginObject();
    jw.AddVariableString("SomeString", "absolutely");
    jw.EndObject();

    dto.Transmit();
  }

  return ezApplication::Continue;
}



EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





