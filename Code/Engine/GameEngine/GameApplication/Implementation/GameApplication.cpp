#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/Console/ConsoleActions.h>
#include <GameEngine/Console/QuakeConsole.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
#  include <GameEngine/Console/ImGuiConsole.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  define USE_IMGUI_CONSOLE 1
#endif

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;
ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> ezGameApplication::s_DefaultDeviceCreator;

ezCVarBool ezGameApplication::cvar_AppVSync("App.VSync", true, ezCVarFlags::Save, "Enables V-Sync");
ezCVarBool ezGameApplication::cvar_AppShowFPS("App.ShowFPS", false, ezCVarFlags::Save, "Show frames per second counter");
ezCVarBool ezGameApplication::cvar_WorldShowObjectOrigins("World.ShowObjectOrigins", false, ezCVarFlags::Default, "Render debug geometry at every game object position");

ezGameApplication::ezGameApplication(const char* szAppName, const char* szProjectPath /*= nullptr*/)
  : ezGameApplicationBase(szAppName)
  , m_sAppProjectPath(szProjectPath)
{
  m_pUpdateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "UpdateWorldsAndExtractViews", ezTaskNesting::Never, ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this));
  m_pUpdateTask->ConfigureTask("GameApplication.Update", ezTaskNesting::Maybe);

  s_pGameApplicationInstance = this;

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

ezResult ezGameApplication::BeforeCoreSystemsStartup()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // before anything else may log, so that '-logfile' contains the entire startup
  Unattended_Setup();
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void ezGameApplication::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // the base class returns early when initialization went wrong, in which case there is no game state
  // and no world to set up
  if (!ShouldApplicationQuit())
  {
    // after the game state, because seeding the random number generators needs the worlds to exist
    Unattended_Start();
  }
#endif
}

void ezGameApplication::BeforeCoreSystemsShutdown()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // while the log writers are still attached, so that the summary ends up in '-logfile'
  Unattended_Finish();
#endif

  SUPER::BeforeCoreSystemsShutdown();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // after the base class, which shuts the logging down
  Unattended_DetachLog();
#endif
}

void ezGameApplication::Run()
{
  SUPER::Run();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  Unattended_CheckTimeout();
#endif
}

void ezGameApplication::StoreScreenshot(ezImage&& image, ezStringView sContext /*= {}*/)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // '-screenshot' names one specific file, so it takes precedence over the path the base class generates
  if (Unattended_StoreScreenshot(image))
    return;
#endif

  SUPER::StoreScreenshot(std::move(image), sContext);
}

//////////////////////////////////////////////////////////////////////////

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

// These options exist to run an application unattended, e.g. as a smoke test from a script.
// They are implemented here, rather than in ezPlayer, so that every application built with the engine
// has them - an exported game as much as the player.
ezCommandLineOptionInt opt_RunFrames("_App", "-runframes", "Quit automatically after this many rendered frames.\nUse this to check that a project starts up and renders at all.", -1, -1);
ezCommandLineOptionFloat opt_Timeout("_App", "-timeout", "Quit automatically after this many seconds, no matter what.\nSafety net in case startup hangs. Sets the return code to 2 when it triggers.", 0.0f, 0.0f);
ezCommandLineOptionPath opt_Screenshot("_App", "-screenshot", "Absolute path to a PNG file to write a screenshot to, right before quitting.\nOnly useful together with -runframes or -timeout.", "");
ezCommandLineOptionPath opt_LogFile("_App", "-logfile", "Absolute path to a text file to write the full log to.", "");
ezCommandLineOptionBool opt_FailOnError("_App", "-failonerror", "Set the return code to 1 if any error was logged during the run.", false);
ezCommandLineOptionFloat opt_FixedTimeStep("_App", "-fixedtimestep",
  "Advance the clock by a fixed 1/N seconds per frame, instead of by the time that really elapsed.\n\
\n\
Together with -seed this makes consecutive runs produce identical frames, which is what a screenshot\n\
has to be for comparing it against a reference image. The application then no longer runs in real time.\n\
\n\
Example:\n\
  -fixedtimestep 30\n",
  0.0f, 0.0f);
ezCommandLineOptionInt opt_Seed("_App", "-seed",
  "Seed for the random number generator of every world, so that random behavior repeats between runs.\n\
Only useful together with -fixedtimestep.",
  -1, -1);

void ezGameApplication::Unattended_Setup()
{
  const ezStringBuilder sLogFile = opt_LogFile.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sLogFile.IsEmpty())
  {
    // uses ezOSFile, so this works before the ezFileSystem is configured
    if (m_UnattendedLogFile.BeginLog(sLogFile).Succeeded())
    {
      m_UnattendedLogFile.SetTimestampMode(ezLog::TimestampMode::TimeOnly);
      m_UnattendedLogToFileID = ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezLogWriter::TextFile::LogMessageHandler, &m_UnattendedLogFile));
    }
    else
    {
      ezLog::Error("Could not open log file '{}' for writing.", sLogFile);
    }
  }

  m_bFailOnError = opt_FailOnError.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (m_bFailOnError)
  {
    m_UnattendedLogErrorCounterID = ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezGameApplication::Unattended_OnLogEvent, this));
  }

  m_iRunFrames = opt_RunFrames.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
  m_UnattendedTimeout = ezTime::MakeFromSeconds(opt_Timeout.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  m_sScreenshotPath = opt_Screenshot.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
  m_iRandomSeed = opt_Seed.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  const float fFixedTimeStepHz = opt_FixedTimeStep.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (fFixedTimeStepHz > 0.0f)
  {
    m_FixedTimeStep = ezTime::MakeFromSeconds(1.0 / fFixedTimeStepHz);
  }

  m_bUnattended = m_iRunFrames >= 0 || m_UnattendedTimeout.IsPositive() || !m_sScreenshotPath.IsEmpty() ||
                  m_bFailOnError || !sLogFile.IsEmpty() || m_FixedTimeStep.IsPositive() || m_iRandomSeed >= 0;
}

void ezGameApplication::Unattended_Start()
{
  // ezTime is only usable once the core systems are up, so the timeout can't start any earlier than this
  m_UnattendedStartTime = ezTime::Now();

  if (m_FixedTimeStep.IsPositive())
  {
    ezClock::GetGlobalClock()->SetFixedTimeStep(m_FixedTimeStep);
  }

  if (m_iRandomSeed >= 0)
  {
    // the worlds that exist at this point are the ones the game state created; a world created later
    // is not covered, seeding it is then up to that code
    for (ezUInt32 i = 0; i < ezWorld::GetWorldCount(); ++i)
    {
      if (ezWorld* pWorld = ezWorld::GetWorld(static_cast<ezUInt8>(i)))
      {
        EZ_LOCK(pWorld->GetWriteMarker());
        pWorld->GetRandomNumberGenerator().Initialize(static_cast<ezUInt64>(m_iRandomSeed));
        pWorld->GetClock().SetFixedTimeStep(m_FixedTimeStep);
      }
    }
  }

  if (m_iRunFrames >= 0 || !m_sScreenshotPath.IsEmpty())
  {
    m_UnattendedExecutionEventsID = m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezGameApplication::Unattended_OnExecutionEvent, this));
  }
}

void ezGameApplication::Unattended_Finish()
{
  if (m_UnattendedExecutionEventsID != 0)
  {
    m_ExecutionEvents.RemoveEventHandler(m_UnattendedExecutionEventsID);
    m_UnattendedExecutionEventsID = 0;
  }

  if (m_bFailOnError && m_iLoggedErrors > 0)
  {
    ezLog::Info("{} errors were logged.", (ezInt32)m_iLoggedErrors);

    // a more specific failure that was already reported wins
    if (GetReturnCode() == 0)
    {
      SetReturnCode(1);
    }
  }
}

void ezGameApplication::Unattended_DetachLog()
{
  if (m_UnattendedLogErrorCounterID != 0)
  {
    ezGlobalLog::RemoveLogWriter(m_UnattendedLogErrorCounterID);
    m_UnattendedLogErrorCounterID = 0;
  }

  if (m_UnattendedLogToFileID != 0)
  {
    ezGlobalLog::RemoveLogWriter(m_UnattendedLogToFileID);
    m_UnattendedLogToFileID = 0;
    m_UnattendedLogFile.EndLog();
  }
}

void ezGameApplication::Unattended_CheckTimeout()
{
  if (m_UnattendedTimeout.IsPositive() && ezTime::Now() - m_UnattendedStartTime > m_UnattendedTimeout)
  {
    ezLog::Error("Timeout of {} seconds reached, quitting.", m_UnattendedTimeout.GetSeconds());
    SetReturnCode(2);
    QuitApplication();
  }
}

void ezGameApplication::Unattended_OnLogEvent(const ezLoggingEventData& e)
{
  if (e.m_EventType == ezLogMsgType::ErrorMsg || e.m_EventType == ezLogMsgType::SeriousWarningMsg)
  {
    m_iLoggedErrors.Increment();
  }
}

void ezGameApplication::Unattended_OnExecutionEvent(const ezGameApplicationExecutionEvent& e)
{
  // BeforePresent, not AfterPresent: Run_FinishFrame() resets the 'take screenshot' flag at the end of each
  // frame, so the request has to be made before the frame is presented
  if (e.m_Type != ezGameApplicationExecutionEvent::Type::BeforePresent)
    return;

  ++m_uiRenderedFrames;

  if (m_bScreenshotRequested)
  {
    // the capture is started during the present of the frame in which it was requested and can only be
    // retrieved one or more frames later, so wait for it, rather than quitting with no (or a broken) file
    if (m_bScreenshotDone)
    {
      QuitApplication();
    }

    return;
  }

  if (m_iRunFrames < 0 || m_uiRenderedFrames < (ezUInt32)m_iRunFrames)
    return;

  if (m_sScreenshotPath.IsEmpty())
  {
    ezLog::Info("Rendered {} frames, quitting.", m_uiRenderedFrames);
    QuitApplication();
    return;
  }

  m_bScreenshotRequested = true;
  TakeScreenshot();
}

bool ezGameApplication::Unattended_StoreScreenshot(ezImage& ref_image)
{
  if (m_sScreenshotPath.IsEmpty())
    return false;

  m_bScreenshotDone = true;

  if (ref_image.Convert(ezImageFormat::R8G8B8_UNORM_SRGB).Failed())
  {
    ezLog::Error("Could not convert the screenshot to RGB8.");
    return true;
  }

  const ezStringView sExtension = ezPathUtils::GetFileExtension(m_sScreenshotPath);
  const ezImageFileFormat* pFormat = ezImageFileFormat::GetWriterFormat(sExtension);

  if (pFormat == nullptr)
  {
    ezLog::Error("No image file format is available to write '{}'.", m_sScreenshotPath);
    return true;
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter memoryWriter(&storage);

  if (pFormat->WriteImage(memoryWriter, ref_image, sExtension).Failed())
  {
    ezLog::Error("Could not encode the screenshot as '{}'.", sExtension);
    return true;
  }

  ezStringBuilder sFolder = m_sScreenshotPath;
  sFolder.PathParentDirectory();

  if (!sFolder.IsEmpty() && ezOSFile::CreateDirectoryStructure(sFolder).Failed())
  {
    ezLog::Error("Could not create the folder for screenshot '{}'.", m_sScreenshotPath);
    return true;
  }

  ezOSFile file;
  if (file.Open(m_sScreenshotPath, ezFileOpenMode::Write).Failed())
  {
    ezLog::Error("Could not open screenshot file '{}' for writing.", m_sScreenshotPath);
    return true;
  }

  ezMemoryStreamReader memoryReader(&storage);
  ezHybridArray<ezUInt8, 4096> chunk;
  chunk.SetCountUninitialized(4096);

  while (const ezUInt64 uiRead = memoryReader.ReadBytes(chunk.GetData(), chunk.GetCount()))
  {
    if (file.Write(chunk.GetData(), uiRead).Failed())
    {
      ezLog::Error("Could not write screenshot file '{}'.", m_sScreenshotPath);
      return true;
    }
  }

  ezLog::Success("Screenshot: '{}'", m_sScreenshotPath);
  return true;
}

#endif

//////////////////////////////////////////////////////////////////////////

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
  const char* s_szOpenInspector = "OpenInspector";
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
    ezConsoleActions::AddAction(s_szInputSet, s_szReloadResourcesAction, "Engine", []()
      { ezResourceManager::ReloadAllResources(false); });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_ShowStats))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);
    ezConsoleActions::AddAction(s_szInputSet, s_szShowFpsAction, "Engine", []()
      { cvar_AppShowFPS = !cvar_AppShowFPS; });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_CaptureProfilingInfo))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);
    ezConsoleActions::AddAction(s_szInputSet, s_szCaptureProfilingAction, "Engine", [this]()
      { TakeProfilingCapture(); });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_CaptureFrame))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF11;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);
    ezConsoleActions::AddAction(s_szInputSet, s_szCaptureFrame, "Engine", [this]()
      { CaptureFrame(); });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_Screenshot))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF12;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);
    ezConsoleActions::AddAction(s_szInputSet, s_szTakeScreenshot, "Engine", [this]()
      { TakeScreenshot(); });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::Dev_OpenInspector))
  {
    config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF10;
    ezInputManager::SetInputActionConfig(s_szInputSet, s_szOpenInspector, config, true);
    ezConsoleActions::AddAction(s_szInputSet, s_szOpenInspector, "Engine", [this]()
      { OpenInspector(); });
  }

  if (flags.IsSet(ezGameApplicationInputFlags::LoadInputConfig))
  {
    ezStringView sConfigFile = ezGameAppInputConfig::s_sConfigFile;

    ezFileReader file;
    if (file.Open(sConfigFile).Succeeded())
    {
      ezTempHybridArray<ezGameAppInputConfig, 32> InputActions;

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

void ezGameApplication::OpenInspector()
{
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  ezProcessOptions opt;

  ezStringBuilder sInspectorPath = ezOSFile::GetApplicationDirectory();
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sInspectorPath.AppendPath("ezInspector.exe");
#  else
  sInspectorPath.AppendPath("ezInspector");
#  endif

  opt.m_sProcess = sInspectorPath;

  ezProcess process;
  if (process.Launch(opt, ezProcessLaunchFlags::Detached).Failed())
  {
    ezLog::Warning("Failed to launch ezInspector.");
  }
#endif
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
  auto pWinMan = ezWindowManager::GetSingleton();

  ezTempHybridArray<ezRegisteredWndHandle, 8> windows;
  pWinMan->GetRegistered(windows);

  for (auto id : windows)
  {
    if (auto pOutput = pWinMan->GetOutputTarget(id))
    {
      // We could call `ExecuteTakeScreenshot` here, which would improve the screenshot latency by one frame. However, all unit test image comparisons would fail.
      EZ_PROFILE_SCOPE("AcquireImage");
      pOutput->AcquireImage();
    }
  }
}

void ezGameApplication::Run_PresentImage()
{
  auto pWinMan = ezWindowManager::GetSingleton();

  ezTempHybridArray<ezRegisteredWndHandle, 8> windows;
  pWinMan->GetRegistered(windows);

  bool bExecutedFrameCapture = false;
  for (auto id : windows)
  {
    if (auto pOutput = pWinMan->GetOutputTarget(id))
    {
      // if we have multiple actors, append the actor name to each screenshot
      ezStringBuilder ctxt;
      if (windows.GetCount() > 1)
      {
        ctxt.Append(" - ", pWinMan->GetName(id));
      }

      ExecuteTakeScreenshot(pOutput, ctxt);

      auto pWindow = pWinMan->GetWindow(id);
      if (pWindow && !bExecutedFrameCapture)
      {
        ExecuteFrameCapture(pWindow->GetNativeWindowHandle(), ctxt);
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

  ezTempHybridArray<ezWorld*, 16> worldsToUpdate;

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

  for (ezUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
  {
    ezWorld* pWorld = worldsToUpdate[i];
    EZ_LOCK(pWorld->GetReadMarker());
    RenderWorldDebugInfos(*pWorld);
  }

  Run_AfterWorldUpdate();

  RenderFps();
  RenderConsole();

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  ezRenderWorld::ExtractMainViews();
}

void ezGameApplication::RenderWorldDebugInfos(const ezWorld& world)
{
  if (cvar_WorldShowObjectOrigins)
  {
    ezUInt32 uiInactive = 0;
    ezUInt32 uiStatic = 0;
    ezUInt32 uiDynamic = 0;

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezTransform tObj = it->GetGlobalTransform();
      tObj.m_vScale.Set(1.0f);

      if (!it->IsActive())
      {
        ++uiInactive;
        ezDebugRenderer::DrawCross(&world, ezVec3::MakeZero(), 0.25f, ezColor::DarkGrey, tObj);
      }
      else if (it->IsDynamic())
      {
        ++uiDynamic;
        ezDebugRenderer::DrawCross(&world, ezVec3::MakeZero(), 0.25f, ezColor::DeepPink, tObj);
      }
      else
      {
        ++uiStatic;
        ezDebugRenderer::DrawCross(&world, ezVec3::MakeZero(), 0.4f, ezColor::DeepSkyBlue, tObj);
      }
    }

    ezDebugRenderer::DrawInfoText(&world, ezDebugTextPlacement::BottomLeft, "WorldStats", ezFmt("Num Objects: {} - {} static / {} dynamic / {} inactive", uiStatic + uiDynamic + uiInactive, uiStatic, uiDynamic, uiInactive));
  }
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
    {
      ezInputManager::SetExclusiveInputSet("Console");
    }
    else
    {
      ezInputManager::SetExclusiveInputSet("");

      if (m_pConsole)
      {
        m_pConsole->SaveInputHistory(":appdata/ConsoleInputHistory.cfg").IgnoreResult();
      }
    }
  }

  ezConsoleActions::HandleInput();

  if (m_pConsole)
  {
    m_pConsole->HandleInput(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == ezKeyState::Pressed)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit("dev-esc");
    }
  }

  return SUPER::Run_ProcessApplicationInput();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);
