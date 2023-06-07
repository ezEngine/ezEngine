#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorManager.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

ezGameApplicationBase* ezGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

ezGameApplicationBase::ezGameApplicationBase(ezStringView sAppName)
  : ezApplication(sAppName)
  , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", ezMakeDelegate(&ezGameApplicationBase::TakeScreenshot, this))
  , m_ConFunc_CaptureFrame("CaptureFrame", "()", ezMakeDelegate(&ezGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

ezGameApplicationBase::~ezGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(ezStringBuilder& out_sString)
{
  const ezDateTime dt = ezTimestamp::CurrentTimestamp();

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true), ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true), ezArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void ezGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public ezTask
  {
  public:
    ezProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask() = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      ezStringBuilder sPath(":appdata/Profiling/", ezApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      ezFileWriter fileWriter;
      if (fileWriter.Open(sPath) == EZ_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        ezLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  ezSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = EZ_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", ezTaskNesting::Never);
  ezProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  ezTaskSystem::StartSingleTask(pWriteProfilingDataTask, ezTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void ezGameApplicationBase::StoreScreenshot(ezImage&& image, ezStringView sContext /*= {} */)
{
  class WriteFileTask final : public ezTask
  {
  public:
    ezImage m_Image;
    ezStringBuilder m_sPath;

    WriteFileTask() = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(ezImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        ezLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  ezSharedPtr<WriteFileTask> pWriteTask = EZ_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", ezTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.Format(":appdata/Screenshots/{0}", ezApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(sContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  ezTaskSystem::StartSingleTask(pWriteTask, ezTaskPriority::LongRunning);
}

void ezGameApplicationBase::ExecuteTakeScreenshot(ezWindowOutputTargetBase* pOutputTarget, ezStringView sContext /* = {} */)
{
  if (m_bTakeScreenshot)
  {
    EZ_PROFILE_SCOPE("ExecuteTakeScreenshot");
    ezImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(std::move(img), sContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void ezGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool ezGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


ezResult ezGameApplicationBase::GetAbsFrameCaptureOutputPath(ezStringBuilder& ref_sOutputPath)
{
  ezStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return ezFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void ezGameApplicationBase::ExecuteFrameCapture(ezWindowHandle targetWindowHandle, ezStringView sContext /*= {} */)
{
  ezFrameCaptureInterface* pCaptureInterface = ezSingletonRegistry::GetSingletonInstance<ezFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  EZ_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      ezStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(sContext);
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

  ezGameApplicationStaticEvent e;
  e.m_Type = ezGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  EZ_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());

  return EZ_SUCCESS;
}

void ezGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  EZ_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  ezGameApplicationStaticEvent e;
  e.m_Type = ezGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  ezActorManager::GetSingleton()->DestroyAllActors(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

ezGameStateBase* ezGameApplicationBase::GetActiveGameStateLinkedToWorld(const ezWorld* pWorld) const
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
  ActivateGameState().IgnoreResult();
}

ezResult ezGameApplicationBase::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void ezGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (WasQuitRequested())
  {
    return;
  }

  ezStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void ezGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void ezGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    ezResourceManager::EngineAboutToShutdown();
    ezResourceManager::ExecuteAllResourceCleanupCallbacks();
    ezResourceManager::FreeAllUnusedResources();
  }
}

void ezGameApplicationBase::BeforeCoreSystemsShutdown()
{
  // shut down all actors and APIs that may have been in use
  if (ezActorManager::GetSingleton() != nullptr)
  {
    ezActorManager::GetSingleton()->Shutdown();
  }

  {
    ezFrameAllocator::Reset();
    ezResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    ezResourceManager::FreeAllUnusedResources();
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

ezApplication::Execution ezGameApplicationBase::Run()
{
  EZ_PROFILE_SCOPE("Run");
  if (m_bWasQuitRequested)
    return ezApplication::Execution::Quit;

  s_bUpdatePluginsExecuted = false;

  ezActorManager::GetSingleton()->Update();

  if (!IsGameUpdateEnabled())
    return ezApplication::Execution::Continue;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    EZ_PROFILE_SCOPE("GameApp_BeginAppTick");
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
    EZ_PROFILE_SCOPE("GameApp_EndAppTick");
    EZ_BROADCAST_EVENT(GameApp_EndAppTick);

    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    EZ_PROFILE_SCOPE("BeforePresent");
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    EZ_PROFILE_SCOPE("Run_Present");
    Run_Present();
  }
  ezClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    EZ_PROFILE_SCOPE("AfterPresent");
    ezGameApplicationExecutionEvent e;
    e.m_Type = ezGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    EZ_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
  return ezApplication::Execution::Continue;
}

void ezGameApplicationBase::Run_InputUpdate()
{
  EZ_PROFILE_SCOPE("Run_InputUpdate");
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
  EZ_PROFILE_SCOPE("Run_UpdatePlugins");
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

void ezGameApplicationBase::Run_Present() {}

void ezGameApplicationBase::Run_FinishFrame()
{
  ezTelemetry::PerFrameUpdate();
  ezResourceManager::PerFrameUpdate();
  ezTaskSystem::FinishFrameTasks();
  ezFrameAllocator::Swap();
  ezProfilingSystem::StartNewFrame();

  // if many messages have been logged, make sure they get written to disk
  ezLog::Flush(100, ezTime::Seconds(10));

  // reset this state
  m_bTakeScreenshot = false;
}

void ezGameApplicationBase::UpdateFrameTime()
{
  // Do not use ezClock for this, it smooths and clamps the timestep
  const ezTime tNow = ezTime::Now();

  static ezTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}

EZ_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
