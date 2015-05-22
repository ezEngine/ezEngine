
#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/FrameAllocator.h>

#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>

#include <System/Window/Window.h>

#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <RendererDX11/Device/DeviceDX11.h>
  typedef ezGALDeviceDX11 ezGALDeviceDefault;
#else
  #include <RendererGL/Device/DeviceGL.h>
  typedef ezGALDeviceGL ezGALDeviceDefault;
#endif

namespace
{
  ezProfilingId g_BeforeWorldUpdateProfilingId = ezProfilingSystem::CreateId("GameApplication.BeforeWorldUpdate");
  ezProfilingId g_AfterWorldUpdateProfilingId = ezProfilingSystem::CreateId("GameApplication.AfterWorldUpdate");
  ezProfilingId g_PresentProfilingId = ezProfilingSystem::CreateId("GameApplication.Present");
  const char* g_szInputSet = "GameApp";
  const char* g_szCloseAppAction = "CloseApp";
  const char* g_szReloadResourcesAction = "ReloadResources";
  const char* g_szCaptureProfilingAction = "CaptureProfiling";
}


ezGameApplication::ezGameApplication(ezGameStateBase& initialGameState)
  : m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
{
  SetCurrentGameState(initialGameState);
  m_bShouldRun = false;
}

ezGameApplication::~ezGameApplication()
{
}

ezGALSwapChainHandle ezGameApplication::AddWindow(ezWindowBase* pWindow)
{
  WindowContext& windowContext = m_Windows.ExpandAndGetRef();
  windowContext.m_pWindow = pWindow;

  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = pWindow;
  desc.m_bAllowScreenshots = true;

  windowContext.m_hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);
  return windowContext.m_hSwapChain;
}

void ezGameApplication::RemoveWindow(ezWindowBase* pWindow)
{
  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    WindowContext& windowContext = m_Windows[i];
    if (windowContext.m_pWindow == pWindow)
    {
      ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_Windows[i].m_hSwapChain);
      break;
    }
  }
}

void ezGameApplication::SetCurrentGameState(ezGameStateBase& currentGameState)
{
  m_pCurrentGameState = &currentGameState;
  m_pCurrentGameState->m_pApplication = this;
}

void ezGameApplication::RequestQuit()
{
  m_bShouldRun = false;
}

void ezGameApplication::AfterEngineInit()
{
  // Setup the logging system
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezTelemetry::CreateServer();

  // init rendering
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = false;
    DeviceInit.m_bDebugDevice = true;

    ezGALDevice* pDevice = EZ_DEFAULT_NEW(ezGALDeviceDefault, DeviceInit);
    EZ_VERIFY(pDevice->Init() == EZ_SUCCESS, "Device init failed!");

    ezGALDevice::SetDefaultDevice(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezRenderContext::ConfigureShaderSystem("DX11_SM40", true);
#else
    ezRenderContext::ConfigureShaderSystem("GL3", true);
#endif
  }

  // load plugins
  {
    EZ_VERIFY(ezPlugin::LoadPlugin("ezInspectorPlugin").Succeeded(), "Could not load Inspector Plugin.");
    
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Could not load HLSL Shader Compiler Plugin.");
  #endif
#endif
  }

  // init clocks
  {
    ezClock::SetNumGlobalClocks();
    ezClock::Get()->SetTimeStepSmoothing(&m_TimeStepSmoother);
  }

  // create some input actions
  {
    ezInputActionConfig config;

    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig(g_szInputSet, g_szCloseAppAction, config, true);

    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
    ezInputManager::SetInputActionConfig(g_szInputSet, g_szReloadResourcesAction, config, true);

    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
    ezInputManager::SetInputActionConfig(g_szInputSet, g_szCaptureProfilingAction, config, true);
  }

  ezStartup::StartupEngine();

  m_pCurrentGameState->Activate();

  // load data dir config
  {
    ezApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();
    appFileSystemConfig.Apply();
  }

  m_bShouldRun = true;
}

void ezGameApplication::BeforeEngineShutdown()
{
  m_pCurrentGameState->Deactivate();

  ezStartup::ShutdownEngine();

  ezFrameAllocator::Reset();
  ezResourceManager::FreeUnusedResources(true);

  // deinit rendering
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
    {
      pDevice->DestroySwapChain(m_Windows[i].m_hSwapChain);
    }

    pDevice->Shutdown();
    EZ_DEFAULT_DELETE(pDevice);
    ezGALDevice::SetDefaultDevice(nullptr);
  }

  ezTelemetry::CloseConnection();
}


ezApplication::ApplicationExecution ezGameApplication::Run()
{
  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    m_Windows[i].m_pWindow->ProcessWindowMessages();
  }

  ///\todo: there should also be a clock per world
  ezClock::UpdateAllGlobalClocks();

  ezTaskGroupID updateTaskID;
  if (ezRenderLoop::GetUseMultithreadedRendering())
  {
    updateTaskID = ezTaskSystem::StartSingleTask(&m_UpdateTask, ezTaskPriority::EarlyThisFrame);
  }
  else
  {
    UpdateWorldsAndExtractViews();
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  {
    pDevice->BeginFrame();

    ezRenderLoop::Render(ezRenderContext::GetDefaultInstance());

    if (ezRenderLoop::GetUseMultithreadedRendering())
    {
      ezTaskSystem::WaitForGroup(updateTaskID);
    }

    ezTelemetry::PerFrameUpdate();
    ezResourceManager::PerFrameUpdate();
    ezTaskSystem::FinishFrameTasks(16.0);

    {
      EZ_PROFILE(g_PresentProfilingId);
      for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
      {
        pDevice->Present(m_Windows[i].m_hSwapChain);
      }
    }

    pDevice->EndFrame();
  }

  ezRenderLoop::FinishFrame();
  ezFrameAllocator::Swap();

  return m_bShouldRun ? ezApplication::Continue : ezApplication::Quit;
}

void ezGameApplication::UpdateInput()
{
  ezInputManager::Update(ezClock::Get()->GetTimeDiff());

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szCloseAppAction) == ezKeyState::Pressed)
  {
    m_bShouldRun = false;
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szReloadResourcesAction) == ezKeyState::Pressed)
  {
    ezResourceManager::ReloadAllResources();
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szCaptureProfilingAction) == ezKeyState::Pressed)
  {
    ezFileWriter fileWriter;
    if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
    {
      ezProfilingSystem::Capture(fileWriter);
    }
  }
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  UpdateInput();

  {
    EZ_PROFILE(g_BeforeWorldUpdateProfilingId);
    m_pCurrentGameState->BeforeWorldUpdate();
  }

  static ezHybridArray<ezWorld*, 16> worldsToUpdate;
  worldsToUpdate.Clear();

  auto views = ezRenderLoop::GetMainViews();
  for (ezUInt32 i = 0; i < views.GetCount(); ++i)
  {
    ezWorld* pWorld = views[i]->GetWorld();
    if (!worldsToUpdate.Contains(pWorld))
      worldsToUpdate.PushBack(pWorld);
  }

  if (ezRenderLoop::GetUseMultithreadedRendering())
  {
    ezTaskGroupID updateWorldsTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    for (ezUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      ezTaskSystem::AddTaskToGroup(updateWorldsTaskID, worldsToUpdate[i]->GetUpdateTask());
    }
    ezTaskSystem::StartTaskGroup(updateWorldsTaskID);
    ezTaskSystem::WaitForGroup(updateWorldsTaskID);
  }
  else
  {
    for (ezUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      ezWorld* pWorld = worldsToUpdate[i];
      EZ_LOCK(pWorld->GetWriteMarker());

      pWorld->Update();
    }
  }

  {
    EZ_PROFILE(g_AfterWorldUpdateProfilingId);
    m_pCurrentGameState->AfterWorldUpdate();
  }

  ezRenderLoop::ExtractMainViews();
}
