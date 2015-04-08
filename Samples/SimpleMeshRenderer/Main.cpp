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
#include <RendererCore/RenderContext/RenderContext.h>

SampleApp::WorldUpdateTask::WorldUpdateTask(SampleApp* pApp)
{
  m_pApp = pApp;
  SetTaskName("GameLoop");
}

void SampleApp::WorldUpdateTask::Execute()
{
  m_pApp->UpdateGame();

  m_pApp->m_View.ExtractData();
}

SampleApp::SampleApp() : m_WorldUpdateTask(this), m_View("MainView")
{
  m_bActiveRenderLoop = false;
  m_pWindow = nullptr;
}

void SampleApp::AfterEngineInit()
{
  EZ_LOG_BLOCK("SampleGameApp::AfterEngineInit");

  // Setup the logging system
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("");

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/Samples/SimpleMeshRenderer/");

  ezStringBuilder sObjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sObjectDir.AppendPath("../../Shared/FreeContent/");

  ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sBaseDir.AppendPath("../../Shared/Data/");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(sBaseDir.GetData(), ezFileSystem::ReadOnly, "Shared");
  ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::AllowWrites, "Game");
  ezFileSystem::AddDataDirectory(sObjectDir.GetData(), ezFileSystem::ReadOnly, "Object");

  ezTelemetry::CreateServer();

  if (ezPlugin::LoadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");
#endif

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);

  InitRendering();

  ezStartup::StartupEngine();

  ezClock::SetNumGlobalClocks();
  ezClock::Get()->SetTimeStepSmoothing(&m_TimeStepSmoother);

  // Map the input keys to actions
  SetupInput();

  CreateGameLevel();

  m_bActiveRenderLoop = true;
}

void SampleApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  DestroyGameLevel();

  DeinitRendering();

  ezStartup::ShutdownEngine();

  ezResourceManager::FreeUnusedResources(true);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);

  EZ_DEFAULT_DELETE(m_pWindow);

  ezTelemetry::CloseConnection();
}

ezApplication::ApplicationExecution SampleApp::Run()
{
  EZ_LOG_BLOCK("SampleGameApp::Run");

  m_pWindow->ProcessWindowMessages();

  if (!m_bActiveRenderLoop)
    return ezApplication::Quit;

  ezClock::UpdateAllGlobalClocks();

  UpdateInputSystem(ezClock::Get()->GetTimeDiff());

  ezTaskGroupID updateTaskID = ezTaskSystem::StartSingleTask(&m_WorldUpdateTask, ezTaskPriority::EarlyThisFrame);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  {
    pDevice->BeginFrame();

    m_View.Render(ezRenderContext::GetDefaultInstance());

    pDevice->Present(pDevice->GetPrimarySwapChain());

    pDevice->EndFrame();
  }

  // needs to be called once per frame
  ezResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  ezTaskSystem::FinishFrameTasks();

  ezTaskSystem::WaitForGroup(updateTaskID);

  ezTelemetry::PerFrameUpdate();

  return ezApplication::Continue;
}


EZ_CONSOLEAPP_ENTRY_POINT(SampleApp);





