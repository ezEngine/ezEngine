#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezMutex ezTaskSystem::s_TaskSystemMutex;
ezUniquePtr<ezTaskSystemState> ezTaskSystem::s_State;
ezUniquePtr<ezTaskSystemThreadState> ezTaskSystem::s_ThreadState;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezTaskSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezTaskSystem::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void ezTaskSystem::Startup()
{
  s_ThreadState = EZ_DEFAULT_NEW(ezTaskSystemThreadState);
  s_State = EZ_DEFAULT_NEW(ezTaskSystemState);

  tl_TaskWorkerInfo.m_WorkerType = ezWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;

  // initialize with the default number of worker threads
  SetWorkerThreadCount();
}

void ezTaskSystem::Shutdown()
{
  StopWorkerThreads();

  s_State.Clear();
  s_ThreadState.Clear();
}

void ezTaskSystem::SetTargetFrameTime(ezTime targetFrameTime)
{
  s_State->m_TargetFrameTime = targetFrameTime;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
