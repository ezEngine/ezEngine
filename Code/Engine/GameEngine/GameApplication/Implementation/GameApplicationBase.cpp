#include <PCH.h>

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Timestamp.h>
#include <GameEngine/GameApplication/GameApplicationBase.h>
#include <GameEngine/Interfaces/FrameCaptureInterface.h>
#include <System/Window/Window.h>

ezGameApplicationBase* ezGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

ezGameApplicationBase::ezGameApplicationBase(const char* szAppName)
    : ezApplication(szAppName)
    , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", ezMakeDelegate(&ezGameApplicationBase::TakeScreenshot, this))
    , m_ConFunc_CaptureFrame("CaptureFrame", "()", ezMakeDelegate(&ezGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

ezGameApplicationBase::~ezGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void ezGameApplicationBase::RequestQuit()
{
  m_bWasQuitRequested = true;
}

//////////////////////////////////////////////////////////////////////////

ezWindowOutputTargetBase* ezGameApplicationBase::AddWindow(ezWindowBase* pWindow)
{
  ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget = CreateWindowOutputTarget(pWindow);
  ezWindowOutputTargetBase* pOutputPtr = pOutputTarget.Borrow();

  AddWindow(pWindow, std::move(pOutputTarget));

  return pOutputPtr;
}

void ezGameApplicationBase::AddWindow(ezWindowBase* pWindow, ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget)
{
  // make sure not to add the same window twice
  RemoveWindow(pWindow);

  WindowContext& windowContext = m_Windows.ExpandAndGetRef();
  windowContext.m_pWindow = pWindow;
  windowContext.m_pOutputTarget = std::move(pOutputTarget);
  windowContext.m_bFirstFrame = true;
}

void ezGameApplicationBase::RemoveWindow(ezWindowBase* pWindow)
{
  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    WindowContext& windowContext = m_Windows[i];
    if (windowContext.m_pWindow == pWindow)
    {
      DestroyWindowOutputTarget(std::move(windowContext.m_pOutputTarget));
      m_Windows.RemoveAtAndCopy(i);
      break;
    }
  }
}

bool ezGameApplicationBase::ProcessWindowMessages()
{
  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    m_Windows[i].m_pWindow->ProcessWindowMessages();
  }

  return !m_Windows.IsEmpty();
}

ezWindowOutputTargetBase* ezGameApplicationBase::GetWindowOutputTarget(const ezWindowBase* pWindow) const
{
  for (auto& windowContext : m_Windows)
  {
    if (windowContext.m_pWindow == pWindow)
    {
      return windowContext.m_pOutputTarget.Borrow();
    }
  }

  return nullptr;
}

void ezGameApplicationBase::SetWindowOutputTarget(const ezWindowBase* pWindow, ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget)
{
  for (auto& windowContext : m_Windows)
  {
    if (windowContext.m_pWindow == pWindow)
    {
      if (windowContext.m_pOutputTarget != nullptr)
      {
        DestroyWindowOutputTarget(std::move(windowContext.m_pOutputTarget));
      }

      windowContext.m_pOutputTarget = std::move(pOutputTarget);
      return;
    }
  }

  EZ_REPORT_FAILURE("The given window is not part of the application!");
}

//////////////////////////////////////////////////////////////////////////

void AppendCurrentTimestamp(ezStringBuilder& out_String)
{
  const ezDateTime dt = ezTimestamp::CurrentTimestamp();

  out_String.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true),
                          ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true),
                          ezArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void ezGameApplicationBase::TakeProfilingCapture()
{
  ezStringBuilder sPath = ":appdata/profiling ";
  AppendCurrentTimestamp(sPath);
  sPath.Append(".json");

  ezFileWriter fileWriter;
  if (fileWriter.Open(sPath) == EZ_SUCCESS)
  {
    ezProfilingSystem::Capture(fileWriter);
    ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  }
  else
  {
    ezLog::Error("Could not write profiling capture to '{0}'.", sPath);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void ezGameApplicationBase::StoreScreenshot(const ezImage& image, const char* szContext /*= nullptr*/)
{
  class WriteFileTask : public ezTask
  {
  public:
    ezImage m_Image;
    ezStringBuilder m_sPath;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(ezImageFormat::R8G8B8_UNORM_SRGB);

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        ezLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  WriteFileTask* pWriteTask = EZ_DEFAULT_NEW(WriteFileTask);
  pWriteTask->m_Image.Reset(image);
  pWriteTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });

  pWriteTask->m_sPath.Format(":appdata/Screenshots/{0} ", ezApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(szContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  ezTaskSystem::StartSingleTask(pWriteTask, ezTaskPriority::LongRunning);
}

void ezGameApplicationBase::ExecuteTakeScreenshot(ezWindowOutputTargetBase* pOutputTarget, const char* szContext /* = nullptr*/)
{
  if (m_bTakeScreenshot)
  {
    ezImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(img, szContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void ezGameApplicationBase::SetContinuousFrameCapture(bool enable)
{
  m_bContinuousFrameCapture = enable;
}

bool ezGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


ezResult ezGameApplicationBase::GetAbsFrameCaptureOutputPath(ezStringBuilder& sOutputPath)
{
  ezStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return ezFileSystem::ResolvePath(sPath, &sOutputPath, nullptr);
}

void ezGameApplicationBase::ExecuteFrameCapture(ezWindowHandle targetWindowHandle, const char* szContext /*= nullptr*/)
{
  ezFrameCaptureInterface* pCaptureInterface =
      ezSingletonRegistry::GetSingletonInstance<ezFrameCaptureInterface>("ezFrameCaptureInterface");
  if (!pCaptureInterface)
  {
    return;
  }

  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      ezStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(szContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      ezStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        ezLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        ezLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

ezResult ezGameApplicationBase::ActivateGameState(ezWorld* pWorld /*= nullptr*/, const ezTransform* pStartPosition /*= nullptr*/)
{
  EZ_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState(pWorld);

  if (m_pGameState == nullptr)
    return EZ_FAILURE;

  m_pWorldLinkedWithGameState = pWorld;
  m_pGameState->OnActivation(pWorld, pStartPosition);
  return EZ_SUCCESS;
}

void ezGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  m_pGameState->OnDeactivation();
  m_pGameState = nullptr;
}

ezGameStateBase* ezGameApplicationBase::GetActiveGameStateLinkedToWorld(ezWorld* pWorld) const
{
  if (m_pWorldLinkedWithGameState == pWorld)
    return m_pGameState.Borrow();

  return nullptr;
}

ezUniquePtr<ezGameStateBase> ezGameApplicationBase::CreateGameState(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("Create Game State");

  ezUniquePtr<ezGameStateBase> pCurState;

  {
    ezInt32 iBestPriority = -1;

    for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (!pRtti->IsDerivedFrom<ezGameStateBase>() || !pRtti->GetAllocator()->CanAllocate())
        continue;

      ezUniquePtr<ezGameStateBase> pState = pRtti->GetAllocator()->Allocate<ezGameStateBase>();

      const ezInt32 iPriority = (ezInt32)pState->DeterminePriority(pWorld);

      if (iPriority > iBestPriority)
      {
        iBestPriority = iPriority;

        pCurState = std::move(pState);
      }
    }
  }

  return pCurState;
}

void ezGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState();
}

void ezGameApplicationBase::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("runtime");

  SUPER::BeforeCoreSystemsStartup();
}

void ezGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void ezGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    ezResourceManager::EngineAboutToShutdown();
    ezResourceManager::ClearAllResourceFallbacks();
    ezResourceManager::FreeUnusedResources(true);
  }
}

void ezGameApplicationBase::BeforeCoreSystemsShutdown()
{
  {
    ezFrameAllocator::Reset();
    ezResourceManager::FreeUnusedResources(true);
  }

  {
    Deinit_ShutdownGraphicsDevice();
    ezResourceManager::FreeUnusedResources(true);
  }

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    ezTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  s_bUpdatePluginsExecuted = true;
}

ezApplication::ApplicationExecution ezGameApplicationBase::Run()
{
  if (m_bWasQuitRequested)
    return ezApplication::Quit;

  s_bUpdatePluginsExecuted = false;

  ProcessWindowMessages();

  ezClock::GetGlobalClock()->Update();

  if (!IsGameUpdateEnabled())
    return ezApplication::Continue;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    EZ_BROADCAST_EVENT(GameApp_BeginAppTick);

    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_InputUpdate();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    EZ_ASSERT_DEV(s_bUpdatePluginsExecuted, "ezGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    EZ_BROADCAST_EVENT(GameApp_EndAppTick);

    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_FinishFrame();

  return ezApplication::Continue;
}

void ezGameApplicationBase::Run_InputUpdate()
{
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool ezGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void ezGameApplicationBase::Run_BeforeWorldUpdate()
{
  EZ_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void ezGameApplicationBase::Run_AfterWorldUpdate()
{
  EZ_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();
  }

  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void ezGameApplicationBase::Run_UpdatePlugins()
{
  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  EZ_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void ezGameApplicationBase::Run_FinishFrame()
{
  ezTelemetry::PerFrameUpdate();
  ezResourceManager::PerFrameUpdate();
  ezTaskSystem::FinishFrameTasks();
  ezFrameAllocator::Swap();

  // reset this state
  m_bTakeScreenshot = false;
}
