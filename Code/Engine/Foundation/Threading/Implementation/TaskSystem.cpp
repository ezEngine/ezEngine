#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezMutex ezTaskSystem::s_TaskSystemMutex;
ezUniquePtr<ezTaskSystemState> ezTaskSystem::s_pState;
ezUniquePtr<ezTaskSystemThreadState> ezTaskSystem::s_pThreadState;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (ezStartup::HasApplicationTag("NoTaskSystem"))
      return;

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
  s_pThreadState = EZ_DEFAULT_NEW(ezTaskSystemThreadState);
  s_pState = EZ_DEFAULT_NEW(ezTaskSystemState);

  tl_TaskWorkerInfo.m_WorkerType = ezWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;

  // initialize with the default number of worker threads
  SetWorkerThreadCount();
}

void ezTaskSystem::Shutdown()
{
  if (s_pThreadState == nullptr)
    return;

  StopWorkerThreads();

  s_pState.Clear();
  s_pThreadState.Clear();
}

void ezTaskSystem::SetTargetFrameTime(ezTime targetFrameTime)
{
  s_pState->m_TargetFrameTime = targetFrameTime;
}

void ezTaskSystem::BroadcastClearThreadLocalsEvent()
{
  for (ezUInt32 i = 0; i < ezWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (ezTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->BroadcastClearThreadLocalsEvent();
      }
    }
  }

  // make sure they have all sent the event
  for (ezUInt32 i = 0; i < ezWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (ezTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->WaitForBroadcastClearTLS();
      }
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
