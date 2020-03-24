#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezMutex ezTaskSystem::s_TaskSystemMutex;
ezTime ezTaskSystem::s_SmoothFrameTime = ezTime::Seconds(1.0 / 40.0); // => 25 ms
ezAtomicInteger32 ezTaskSystem::s_IdleWorkerThreads[ezWorkerThreadType::ENUM_COUNT];
ezAtomicInteger32 ezTaskSystem::s_BlockedWorkerThreads[ezWorkerThreadType::ENUM_COUNT];
ezDynamicArray<ezTaskWorkerThread*> ezTaskSystem::s_WorkerThreads[ezWorkerThreadType::ENUM_COUNT];
ezAtomicInteger32 ezTaskSystem::s_iNumWorkerThreads[ezWorkerThreadType::ENUM_COUNT];
ezDeque<ezTaskGroup> ezTaskSystem::s_TaskGroups;
ezList<ezTaskSystem::TaskData> ezTaskSystem::s_Tasks[ezTaskPriority::ENUM_COUNT];
ezUInt32 ezTaskSystem::s_MaxWorkerThreadsToUse[ezWorkerThreadType::ENUM_COUNT] = {};

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
  tl_TaskWorkerInfo.m_WorkerType = ezWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;
}

void ezTaskSystem::Shutdown()
{
  StopWorkerThreads();

  for (ezUInt32 i = 0; i < ezTaskPriority::ENUM_COUNT; ++i)
  {
    s_Tasks[i].Clear();
    s_Tasks[i].Compact();
  }

  s_TaskGroups.Clear();
  s_TaskGroups.Compact();
}

void ezTaskSystem::SetTargetFrameTime(ezTime smoothFrame)
{
  s_SmoothFrameTime = smoothFrame;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
