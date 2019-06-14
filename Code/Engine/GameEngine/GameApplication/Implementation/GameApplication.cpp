#include <GameEnginePCH.h>

#include <Core/ActorSystem/Actor2.h>
#include <Core/ActorSystem/ActorManager2.h>
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
#include <GameEngine/Console/Console.h>
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

ezCVarBool CVarEnableVSync("g_VSync", false, ezCVarFlags::Save, "Enables V-Sync");
ezCVarBool CVarShowFPS("g_ShowFPS", false, ezCVarFlags::Save, "Show frames per second counter");

ezGameApplication::ezGameApplication(const char* szAppName, const char* szProjectPath /*= nullptr*/)
  : ezGameApplicationBase(szAppName)
  , m_sAppProjectPath(szProjectPath)
  , m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
{
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;

  m_pConsole = EZ_DEFAULT_NEW(ezConsole);
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

void ezGameApplication::ReinitializeInputConfig()
{
  Init_ConfigureInput();
}

ezResult ezGameApplication::BeforeCoreSystemsStartup()
{
  SUPER::BeforeCoreSystemsStartup();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_AppType == ezGameApplicationType::StandAloneMixedReality || m_AppType == ezGameApplicationType::EmbeddedInToolMixedReality)
  {
    m_pMixedRealityFramework = EZ_DEFAULT_NEW(ezMixedRealityFramework, nullptr);
  }
#endif

  return EZ_SUCCESS;
}

ezString ezGameApplication::FindProjectDirectory() const
{
  EZ_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built in project directory passed to the "
                                                  "ezGameApplication constructor, or m_sAppProjectPath must be set manually before doing "
                                                  "project setup, or ezGameApplication::FindProjectDirectory() must be overridden.");

  if (ezPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the SDK special directory
  {
    ezStringBuilder relToSdk(">sdk/", m_sAppProjectPath);
    ezStringBuilder absToSdk;
    if (ezFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (ezOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

  ezStringBuilder result;
  if (ezFileSystem::FindFolderWithSubPath(ezOSFile::GetApplicationDirectory(), m_sAppProjectPath, result).Failed())
  {
    ezLog::Error("Could not find the project directory.");
  }

  return result;
}

bool ezGameApplication::IsGameUpdateEnabled() const
{
  return ezRenderWorld::GetMainViews().GetCount() > 0;
}

void ezGameApplication::Run_WorldUpdateAndRender()
{
  ezRenderWorld::BeginFrame();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // On most platforms it doesn't matter that much how early this happens.
  // But on HoloLens this executes something that needs to be done at the right time,
  // for the reprojection to work properly.
  pDevice->BeginFrame();

  ezTaskGroupID updateTaskID;
  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = ezTaskSystem::StartSingleTask(&m_UpdateTask, ezTaskPriority::EarlyThisFrame);
  }
  else
  {
    UpdateWorldsAndExtractViews();
  }

  RenderFps();
  RenderConsole();

  ezRenderWorld::Render(ezRenderContext::GetDefaultInstance());

  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    EZ_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    ezTaskSystem::WaitForGroup(updateTaskID);
  }

  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  if (ezRenderWorld::GetFrameCounter() < 10)
    ezLog::Debug("Finishing Frame: {0}", ezRenderWorld::GetFrameCounter());

  {
    ezHybridArray<ezActor*, 8> allActors;
    ezActorManager::GetSingleton()->GetAllActors(allActors);

    for (ezActor* pActor : allActors)
    {
      // Ignore actors without an output target
      if (auto pOutput = pActor->m_pWindowOutputTarget.Borrow())
      {
        // if we have multiple actors, append the actor name to each screenshot
        ezStringBuilder ctxt;
        if (allActors.GetCount() > 1)
        {
          ctxt.Append(" - ", pActor->GetName());
        }

        ExecuteTakeScreenshot(pOutput, ctxt);

        if (pActor->m_pWindow)
        {
          ExecuteFrameCapture(pActor->m_pWindow->GetNativeWindowHandle(), ctxt);
        }

        pOutput->Present(CVarEnableVSync);
      }
    }
  }

  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  ezGALDevice::GetDefaultDevice()->EndFrame();
  ezRenderWorld::EndFrame();
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  ezStringBuilder sb;
  sb.Format("FRAME {}", ezRenderWorld::GetFrameCounter());
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

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  ezRenderWorld::ExtractMainViews();
}

void ezGameApplication::RenderFps()
{
  // Do not use ezClock for this, it smooths and clamps the timestep
  const ezTime tNow = ezTime::Now();

  static ezTime fAccumTime;
  static ezTime fElapsedTime;
  static ezTime tLast = tNow;
  const ezTime tDiff = tNow - tLast;
  fAccumTime += tDiff;
  tLast = tNow;

  if (fAccumTime >= ezTime::Seconds(0.5))
  {
    fAccumTime -= ezTime::Seconds(0.5);

    fElapsedTime = tDiff;
  }

  if (CVarShowFPS)
  {
    if (const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView))
    {
      ezStringBuilder sFps;
      sFps.Format(
        "{0} fps, {1} ms", ezArgF(1.0 / fElapsedTime.GetSeconds(), 1, false, 4), ezArgF(fElapsedTime.GetSeconds() * 1000.0, 1, false, 4));

      ezInt32 viewHeight = (ezInt32)(pView->GetViewport().height);

      ezDebugRenderer::Draw2DText(pView->GetHandle(), sFps, ezVec2I32(10, viewHeight - 10 - 16), ezColor::White);
    }
  }
}

void ezGameApplication::RenderConsole()
{
  if (!m_bShowConsole || !m_pConsole)
    return;

  const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  ezViewHandle hView = pView->GetHandle();

  const float fViewWidth = pView->GetViewport().width;
  const float fViewHeight = pView->GetViewport().height;
  const float fTextHeight = 20.0f;
  const float fConsoleHeight = (fViewHeight / 2.0f);
  const float fBorderWidth = 3.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - fTextHeight - (2.0f * fBorderWidth);

  const ezInt32 iTextHeight = (ezInt32)fTextHeight;
  const ezInt32 iTextLeft = (ezInt32)(fBorderWidth);

  {
    ezColor backgroundColor(0.3f, 0.3f, 0.3f, 0.7f);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

    ezColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    ezDebugRenderer::Draw2DRectangle(
      hView, ezRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    ezDebugRenderer::Draw2DRectangle(hView,
      ezRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f,
      foregroundColor);
  }

  {
    EZ_LOCK(m_pConsole->GetMutex());

    auto& consoleStrings = m_pConsole->GetConsoleStrings();

    ezUInt32 uiNumConsoleLines = (ezUInt32)(ezMath::Ceil(fConsoleTextAreaHeight / fTextHeight));
    ezInt32 iFirstLinePos = (ezInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
    ezInt32 uiFirstLine = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
    ezInt32 uiSkippedLines = ezMath::Max(uiFirstLine - (ezInt32)consoleStrings.GetCount() + 1, 0);

    for (ezUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      ezDebugRenderer::Draw2DText(
        hView, consoleString.m_sText, ezVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.m_TextColor);
    }

    ezStringView sInputLine(m_pConsole->GetInputLine());
    ezDebugRenderer::Draw2DText(hView, sInputLine, ezVec2I32(iTextLeft, (ezInt32)(fConsoleTextAreaHeight + fBorderWidth)), ezColor::White);

    if (ezMath::Fraction(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float fCaretPosition = (float)m_pConsole->GetCaretPosition();
      ezColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      ezDebugRenderer::Draw2DRectangle(hView,
        ezRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f),
        0.0f, caretColor);
    }
  }
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

void ezGameApplication::Init_ConfigureInput()
{
  ezInputActionConfig config;

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szCloseAppAction, config, true);

  // the tilde has problematic behavior on keyboards where it is a hat (^)
  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF1;
  ezInputManager::SetInputActionConfig("Console", s_szShowConsole, config, true);

  // in the editor we cannot use F5, because that is already 'run application'
  // so we use F4 there, and it should be consistent here
  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF4;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szReloadResourcesAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF11;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF12;
  ezInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);

  {
    ezFileReader file;
    if (file.Open(":project/InputConfig.ddl").Succeeded())
    {
      ezHybridArray<ezGameAppInputConfig, 32> InputActions;

      ezGameAppInputConfig::ReadFromDDL(file, InputActions);
      ezGameAppInputConfig::ApplyAll(InputActions);
    }
  }
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
      ezInputManager::SetExclusiveInputSet("");
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szShowFpsAction) == ezKeyState::Pressed)
  {
    CVarShowFPS = !CVarShowFPS;
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
    m_pConsole->DoDefaultInputHandling(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == ezKeyState::Pressed)
  {
    RequestQuit();
  }

  return true;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);
