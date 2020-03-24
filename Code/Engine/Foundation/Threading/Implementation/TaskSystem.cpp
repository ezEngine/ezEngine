#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezMutex ezTaskSystem::s_TaskSystemMutex;
double ezTaskSystem::s_fSmoothFrameMS = 1000.0 / 40.0; // => 25 ms
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

void ezTaskSystem::SetTargetFrameTime(double fSmoothFrameMS)
{
  s_fSmoothFrameMS = fSmoothFrameMS;
}

void ezTaskSystem::WaitForCondition(ezDelegate<bool()> condition)
{
  EZ_PROFILE_SCOPE("WaitForCondition");

  EZ_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != ezWorkerThreadType::MainThread;

  while (!condition())
  {
    if (!HelpExecutingTasks(ezTaskGroupID()))
    {
      if (bAllowSleep)
      {
        s_BlockedWorkerThreads[ThreadTaskType].Increment();

        const ezWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == ezWorkerThreadType::Unknown) ? ezWorkerThreadType::ShortTasks : ThreadTaskType;

        WakeUpThreads(typeToWakeUp, 1);

        while (!condition())
        {
          // TODO: busy loop for now
          ezThreadUtils::YieldTimeSlice();
        }

        s_BlockedWorkerThreads[ThreadTaskType].Decrement();
        break;
      }
      else
      {
        ezThreadUtils::YieldTimeSlice();
      }
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
