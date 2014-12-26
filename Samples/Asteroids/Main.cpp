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
#include <Core/ResourceManager/ResourceManager.h>

#include <RendererGL/Device/DeviceGL.h>
#include <RendererCore/Pipeline/RenderPipeline.h>

SampleGameApp::WorldUpdateTask::WorldUpdateTask(SampleGameApp* pApp)
{
  m_pApp = pApp;
  SetTaskName("GameLoop");
}

void SampleGameApp::WorldUpdateTask::Execute()
{
  m_pApp->UpdateInput(ezClock::Get()->GetTimeDiff());
  m_pApp->m_pLevel->Update();

  m_pApp->m_View.ExtractData();
}

SampleGameApp::SampleGameApp() : 
  m_WorldUpdateTask(this), m_View("MainView")
{
  m_bActiveRenderLoop = false;
  m_pLevel = nullptr;
  m_pWindow = nullptr;
}

void SampleGameApp::AfterEngineInit()
{
  EZ_LOG_BLOCK("SampleGameApp::AfterEngineInit");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory());

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/FreeContent/Asteroids/");

  ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::AllowWrites, "Asteroids Content");

  ezTelemetry::CreateServer();

  if (ezPlugin::LoadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");
#endif

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

  DestroyGameLevel();

  ezStartup::ShutdownEngine();

  ezRenderPipeline* pRenderPipeline = m_View.GetRenderPipeline();
  EZ_DEFAULT_DELETE(pRenderPipeline);

  while (ezResourceManager::FreeUnusedResources() > 0) { }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleGameApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_pWindow->ProcessWindowMessages();

  if (!m_bActiveRenderLoop)
    return ezApplication::Quit;

  ezClock::UpdateAllGlobalClocks();

  ezTaskGroupID updateTaskID = ezTaskSystem::StartSingleTask(&m_WorldUpdateTask, ezTaskPriority::EarlyThisFrame);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  {
    pDevice->BeginFrame();

    m_View.Render(pDevice->GetPrimaryContext());

    pDevice->Present(pDevice->GetPrimarySwapChain());

    pDevice->EndFrame();
  }  

  ezTaskSystem::FinishFrameTasks(16.6);
  ezTaskSystem::WaitForGroup(updateTaskID);

  ezTelemetry::PerFrameUpdate();

  return ezApplication::Continue;
}


EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);





