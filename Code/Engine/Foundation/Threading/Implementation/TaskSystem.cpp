#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezUniquePtr<ezTaskSystemThreadState> ezTaskSystem::s_ThreadState;

ezMutex ezTaskSystem::s_TaskSystemMutex;
ezTime ezTaskSystem::s_SmoothFrameTime = ezTime::Seconds(1.0 / 40.0); // => 25 ms
ezDeque<ezTaskGroup> ezTaskSystem::s_TaskGroups;

ezList<ezTaskSystem::TaskData> ezTaskSystem::s_Tasks[ezTaskPriority::ENUM_COUNT];

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

  s_ThreadState.Clear();
}

void ezTaskSystem::SetTargetFrameTime(ezTime smoothFrame)
{
  s_SmoothFrameTime = smoothFrame;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
