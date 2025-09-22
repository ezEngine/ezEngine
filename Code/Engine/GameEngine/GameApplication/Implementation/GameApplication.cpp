#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/Console/QuakeConsole.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;
ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> ezGameApplication::s_DefaultDeviceCreator;

ezCVarBool ezGameApplication::cvar_AppVSync("App.VSync", true, ezCVarFlags::Save, "Enables V-Sync");
ezCVarBool ezGameApplication::cvar_AppShowFPS("App.ShowFPS", false, ezCVarFlags::Save, "Show frames per second counter");

ezGameApplication::ezGameApplication(const char* szAppName, const char* szProjectPath /*= nullptr*/)
  : ezGameApplicationBase(szAppName)
  , m_sAppProjectPath(szProjectPath)
{
  m_pUpdateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "UpdateWorldsAndExtractViews", ezTaskNesting::Never, ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this));
  m_pUpdateTask->ConfigureTask("GameApplication.Update", ezTaskNesting::Maybe);

  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;

#if USE_IMGUI_CONSOLE
  m_pConsole = EZ_DEFAULT_NEW(ezImGuiConsole);
#else
  m_pConsole = EZ_DEFAULT_NEW(ezQuakeConsole);
#endif

  if (m_pConsole)
  {
    ezConsole::SetMainConsole(m_pConsole.Borrow());
  }
}

ezGameApplication::~ezGameApplication()
{
  s_pGameApplicationInstance = nullptr;
}

// static
void ezGameApplication::SetOverrideDefaultDeviceCreator(ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
}

namespace
{
  const char* s_szInputSet = "GameApp";
  const char* s_szCloseAppAction = "CloseApp";
  const char* s_szShowConsole = "ShowConsole";
  const char* s_szShowFpsAction = "ShowFps";
  const char* s_szReloadResourcesAction = "ReloadResources";
  const char* s_szCaptureProfilingAction = "CaptureProfiling";
  const char* s_szCaptureFrame = "CaptureFrame";
  const char* s_szTakeScreenshot = "TakeScreenshot";
} // namespace


void ezGameApplication::RegisterGameApplicationInputActions(ezBitflags<ezGameApplicationInputFlags> flags)
{
  ezInputActionConfig config;

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_EscapeToClose))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szCloseAppAction, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_Console))
  {
    // the tilde has problematic behavior on keyboards where it is a hat (^)
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF1;
    ezInputManager::SetInputActionConfig("Console", s_szShowConsole, config, true);

    if (m_pConsole)
    {
      m_pConsole->LoadInputHistory(":appdata/ConsoleInputHistory.cfg");
    }
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_ReloadResources))
  {
    // in the editor we cannot use F5, because that is already 'run application'
    // so we use F4 there, and it should be consistent here
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF4;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szReloadResourcesAction, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_ShowStats))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_CaptureProfilingInfo))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_CaptureFrame))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF11;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_Screenshot))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF12;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);
  }

  if (flags.IsSet(ezGameApplicationInputFlags::LoadInputConfig))
  {
    ezStringView sConfigFile = ezGameAppInputConfig::s_sConfigFile;

    ezFileReader file;
    if (file.Open(sConfigFile).Succeeded())
    {
      ezHybridArray<ezGameAppInputConfig, 32> InputActions;

      ezGameAppInputConfig::ReadFromDDL(file, InputActions);
      ezGameAppInputConfig::ApplyAll(InputActions);
    }
  }
}

ezString ezGameApplication::FindProjectDirectory() const
{
  EZ_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built-in project directory passed to the ezGameApplication constructor, or m_sAppProjectPath must be set manually before doing project setup, or ezGameApplication::FindProjectDirectory() must be overridden.");

  if (ezPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the SDK special directory
  {
    ezStringBuilder relToSdk(m_sAppProjectPath);

    if (!relToSdk.StartsWith_NoCase(">sdk/"))
    {
      relToSdk.Prepend(">sdk/");
    }

    ezStringBuilder absToSdk;
    if (ezFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (ezOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

  ezStringBuilder result;
  if (ezFileSystem::FindFolderWithSubPath(result, ezOSFile::GetApplicationDirectory(), m_sAppProjectPath).Failed())
  {
    ezLog::Error("Could not find the project directory.");
  }

  return result;
}

ezGameUpdateMode ezGameApplication::GetGameUpdateMode() const
{
  const bool bViewsScheduled = !ezRenderWorld::GetMainViews().IsEmpty();
  const bool bRenderingScheduled = ezRenderWorld::IsRenderingScheduled();
  if (bViewsScheduled)
  {
    return ezGameUpdateMode::UpdateInputAndRender;
  }
  return bRenderingScheduled ? ezGameUpdateMode::Render : ezGameUpdateMode::Skip;
}

void ezGameApplication::Run_WorldUpdateAndRender()
{
  EZ_PROFILE_SCOPE("Run_WorldUpdateAndRender");
  // If multi-threaded rendering is disabled, the same content is updated/extracted and rendered in the same frame.
  // As ezRenderWorld::BeginFrame applies the render pipeline properties that were set during the update phase, it needs to be done after update/extraction but before rendering.
  if (!ezRenderWorld::GetUseMultithreadedRendering())
  {
    UpdateWorldsAndExtractViews();
  }

  ezRenderWorld::BeginFrame();

  ezTaskGroupID updateTaskID;
  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = ezTaskSystem::StartSingleTask(m_pUpdateTask, ezTaskPriority::EarlyThisFrame);
  }

  ezRenderWorld::Render(ezRenderContext::GetDefaultInstance());

  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    EZ_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    ezTaskSystem::WaitForGroup(updateTaskID);
  }
}

void ezGameApplication::Run_AcquireImage()
{
  ezHybridArray<ezActor*, 8> allActors;
  ezActorManager::GetSingleton()->GetAllActors(allActors);

  for (ezActor* pActor : allActors)
  {
    EZ_PROFILE_SCOPE(pActor->GetName());

    ezActorPluginWindow* pWindowPlugin = pActor->GetPlugin<ezActorPluginWindow>();

    if (pWindowPlugin == nullptr)
      continue;

    // Ignore actors without an output target
    if (auto pOutput = pWindowPlugin->GetOutputTarget())
    {
      EZ_PROFILE_SCOPE("AcquireImage");
      pOutput->AcquireImage();
    }
  }
}

void ezGameApplication::Run_PresentImage()
{
  ezHybridArray<ezActor*, 8> allActors;
  ezActorManager::GetSingleton()->GetAllActors(allActors);

  bool bExecutedFrameCapture = false;
  for (ezActor* pActor : allActors)
  {
    EZ_PROFILE_SCOPE(pActor->GetName());

    ezActorPluginWindow* pWindowPlugin = pActor->GetPlugin<ezActorPluginWindow>();

    if (pWindowPlugin == nullptr)
      continue;

    // Ignore actors without an output target
    if (auto pOutput = pWindowPlugin->GetOutputTarget())
    {
      // if we have multiple actors, append the actor name to each screenshot
      ezStringBuilder ctxt;
      if (allActors.GetCount() > 1)
      {
        ctxt.Append(" - ", pActor->GetName());
      }

      ExecuteTakeScreenshot(pOutput, ctxt);

      if (pWindowPlugin->GetWindow() && !bExecutedFrameCapture)
      {
        ExecuteFrameCapture(pWindowPlugin->GetWindow()->GetNativeWindowHandle(), ctxt);
        bExecutedFrameCapture = true;
      }

      EZ_PROFILE_SCOPE("PresentImage");
      pOutput->PresentImage(cvar_AppVSync);
    }
  }
}

void ezGameApplication::Run_FinishFrame()
{
  ezRenderWorld::EndFrame();

  SUPER::Run_FinishFrame();
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  ezStringBuilder sb;
  sb.SetFormat("UPDATE FRAME {}", ezRenderWorld::GetFrameCounter());
  EZ_PROFILE_SCOPE(sb.GetData());

  Run_BeforeWorldUpdate();

  static ezHybridArray<ezWorld*, 16> worldsToUpdate;
  worldsToUpdate.Clear();

  auto mainViews = ezRenderWorld::GetMainViews();
  for (auto hView : mainViews)
  {
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(hView, pView))
    {
      ezWorld* pWorld = pView->GetWorld();

      if (pWorld != nullptr && !worldsToUpdate.Contains(pWorld))
      {
        worldsToUpdate.PushBack(pWorld);
      }
    }
  }

  if (ezRenderWorld::GetUseMultithreadedRendering())
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

  Run_AfterWorldUpdate();

  RenderFps();
  RenderConsole();

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  ezRenderWorld::ExtractMainViews();
}

void ezGameApplication::RenderFps()
{
  EZ_PROFILE_SCOPE("RenderFps");
  // Do not use ezClock for this, it smooths and clamps the timestep

  static ezTime tAccumTime;
  static ezTime tDisplayedFrameTime = m_FrameTime;
  static ezUInt32 uiFrames = 0;
  static ezUInt32 uiFPS = 0;

  ++uiFrames;
  tAccumTime += m_FrameTime;

  if (tAccumTime >= ezTime::MakeFromSeconds(0.5))
  {
    tAccumTime -= ezTime::MakeFromSeconds(0.5);
    tDisplayedFrameTime = m_FrameTime;

    uiFPS = uiFrames * 2;
    uiFrames = 0;
  }

  if (cvar_AppShowFPS)
  {
    if (const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView))
    {
      ezDebugRenderer::DrawInfoText(pView->GetHandle(), ezDebugTextPlacement::BottomLeft, "FPS", ezFmt("{0} fps, {1} ms", uiFPS, ezArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)));
    }
  }
}

void ezGameApplication::RenderConsole()
{
  if (!m_pConsole)
    return;

  EZ_PROFILE_SCOPE("RenderConsole");

  m_pConsole->RenderConsole(m_bShowConsole);
}

bool ezGameApplication::Run_ProcessApplicationInput()
{
  // the show console command must be in the "Console" input set, because we are using that for exclusive input when the console is open
  if (ezInputManager::GetInputActionState("Console", s_szShowConsole) == ezKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;

    if (m_bShowConsole)
      ezInputManager::SetExclusiveInputSet("Console");
    else
    {
      ezInputManager::SetExclusiveInputSet("");

      if (m_pConsole)
      {
        m_pConsole->SaveInputHistory(":appdata/ConsoleInputHistory.cfg").IgnoreResult();
      }
    }
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szShowFpsAction) == ezKeyState::Pressed)
  {
    cvar_AppShowFPS = !cvar_AppShowFPS;
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szReloadResourcesAction) == ezKeyState::Pressed)
  {
    ezResourceManager::ReloadAllResources(false);
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szTakeScreenshot) == ezKeyState::Pressed)
  {
    TakeScreenshot();
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCaptureProfilingAction) == ezKeyState::Pressed)
  {
    TakeProfilingCapture();
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCaptureFrame) == ezKeyState::Pressed)
  {
    CaptureFrame();
  }

  if (m_pConsole)
  {
    m_pConsole->HandleInput(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == ezKeyState::Pressed)
  {
    RequestApplicationQuit();
  }

  return true;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);
