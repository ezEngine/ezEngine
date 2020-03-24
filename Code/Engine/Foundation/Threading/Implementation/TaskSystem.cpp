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

ezTaskGroupID ezTaskSystem::StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority, ezTaskGroupID Dependency)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority);
  AddTaskGroupDependency(Group, Dependency);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

ezTaskGroupID ezTaskSystem::StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

void ezTaskSystem::ReprioritizeFrameTasks()
{
  // There should usually be no 'this frame tasks' left at this time
  // however, while we waited to enter the lock, such tasks might have appeared
  // In this case we move them into the highest-priority 'this frame' queue, to ensure they will be executed asap
  for (ezUInt32 i = (ezUInt32)ezTaskPriority::ThisFrame; i <= (ezUInt32)ezTaskPriority::LateThisFrame; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'this frame' tasks into the 'early this frame' queue
    while (it.IsValid())
    {
      s_Tasks[ezTaskPriority::EarlyThisFrame].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::EarlyNextFrame; i <= (ezUInt32)ezTaskPriority::LateNextFrame; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'next frame' tasks into the 'this frame' queues
    while (it.IsValid())
    {
      s_Tasks[i - 3].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::In2Frames; i <= (ezUInt32)ezTaskPriority::In9Frames; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'in N frames' tasks into the 'in N-1 frames' queues
    // moves 'In2Frames' into 'LateNextFrame'
    while (it.IsValid())
    {
      s_Tasks[i - 1].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }
}

void ezTaskSystem::ExecuteSomeFrameTasks(ezUInt32 uiSomeFrameTasks, double fSmoothFrameMS)
{
  if (uiSomeFrameTasks == 0)
    return;

  EZ_PROFILE_SCOPE("SomeFrameMainThreadTasks");

  // 'SomeFrameMainThread' tasks are usually used to upload resources that have been loaded in the background
  // they do not need to be executed right away, but the earlier, the better

  // as long as the frame time is short enough, execute tasks that need to be done on the main thread
  // on fast machines that means that these tasks are finished as soon as possible and users will see the results quickly

  // if the frame time spikes, we can skip this a few times, to try to prevent further slow downs
  // however in such instances, the 'frame time threshold' will increase and thus the chance that we skip this entirely becomes lower over
  // time that guarantees some progress, even if the frame rate is constantly low

  static double s_fFrameTimeThreshold = fSmoothFrameMS;

  static ezTime s_LastFrame; // initializes to zero -> very large frame time difference at first

  ezTime CurTime = ezTime::Now();
  ezTime LastTime = s_LastFrame;
  s_LastFrame = CurTime;

  // as long as we have a smooth frame rate, execute as many of these tasks, as possible
  while ((uiSomeFrameTasks > 0) && ((CurTime - LastTime).GetMilliseconds() < fSmoothFrameMS))
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_fFrameTimeThreshold = fSmoothFrameMS;

    if (!ExecuteTask(ezTaskPriority::SomeFrameMainThread, ezTaskPriority::SomeFrameMainThread, false, ezTaskGroupID(), nullptr))
      return; // nothing left to do

    CurTime = ezTime::Now();
    --uiSomeFrameTasks;
  }

  // nothing left to do
  if (uiSomeFrameTasks == 0)
    return;

  if ((CurTime - LastTime).GetMilliseconds() < s_fFrameTimeThreshold)
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_fFrameTimeThreshold = fSmoothFrameMS;

    ExecuteTask(ezTaskPriority::SomeFrameMainThread, ezTaskPriority::SomeFrameMainThread, false, ezTaskGroupID(), nullptr);
  }
  else
  {
    // increase the threshold by 5 ms
    // this means that when the frame rate is too low, we can ignore these tasks for a few frames
    // and thus prevent decreasing the frame rate even further
    // however we increase the time threshold, at which we skip this, further and further
    // therefore at some point we will execute at least one such task, no matter how low the frame rate is
    // this guarantees at least some progress with these tasks
    s_fFrameTimeThreshold += 5.0;

    //  25 ms -> 40 FPS
    //  30 ms -> 33 FPS
    //  35 ms -> 28 FPS
    //  40 ms -> 25 FPS
    //  45 ms -> 22 FPS
    //  50 ms -> 20 FPS
    //  55 ms -> 18 FPS
    //  60 ms -> 16 FPS
    //  65 ms -> 15 FPS
    //  70 ms -> 14 FPS
    //  75 ms -> 13 FPS
  }
}

void ezTaskSystem::DetermineTasksToExecuteOnThread(ezTaskPriority::Enum& out_FirstPriority, ezTaskPriority::Enum& out_LastPriority)
{
  switch (tl_TaskWorkerInfo.m_WorkerType)
  {
    case ezWorkerThreadType::MainThread:
    {
      out_FirstPriority = ezTaskPriority::ThisFrameMainThread;
      out_LastPriority = ezTaskPriority::SomeFrameMainThread;
      break;
    }

    case ezWorkerThreadType::FileAccess:
    {
      out_FirstPriority = ezTaskPriority::FileAccessHighPriority;
      out_LastPriority = ezTaskPriority::FileAccess;
      break;
    }

    case ezWorkerThreadType::LongTasks:
    {
      out_FirstPriority = ezTaskPriority::LongRunningHighPriority;
      out_LastPriority = ezTaskPriority::LongRunning;
      break;
    }

    case ezWorkerThreadType::ShortTasks:
    {
      out_FirstPriority = ezTaskPriority::EarlyThisFrame;
      out_LastPriority = ezTaskPriority::In9Frames;
      break;
    }

    case ezWorkerThreadType::Unknown:
    {
      // probably a thread not launched through ez
      out_FirstPriority = ezTaskPriority::EarlyThisFrame;
      out_LastPriority = ezTaskPriority::In9Frames;
      break;
    }

    default:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}

void ezTaskSystem::SetTargetFrameTime(double fSmoothFrameMS)
{
  s_fSmoothFrameMS = fSmoothFrameMS;
}

void ezTaskSystem::FinishFrameTasks()
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function must be executed on the main thread.");

  // make sure all 'main thread' and 'short' tasks are either finished or being worked on by other threads already
  {
    while (true)
    {
      // Prefer to work on main-thread tasks
      if (ExecuteTask(ezTaskPriority::ThisFrameMainThread, ezTaskPriority::ThisFrameMainThread, false, ezTaskGroupID(), nullptr))
      {
        continue;
      }

      // if there are none, help out with the other tasks for this frame
      if (ExecuteTask(ezTaskPriority::EarlyThisFrame, ezTaskPriority::LateThisFrame, false, ezTaskGroupID(), nullptr))
      {
        continue;
      }

      break;
    }
  }

  ezUInt32 uiSomeFrameTasks = 0;

  // all the important tasks for this frame should be finished or worked on by now
  // so we can now re-prioritize the tasks for the next frame
  {
    EZ_LOCK(s_TaskSystemMutex);

    // get this info once, it won't shrink (but might grow) while we are outside the lock
    uiSomeFrameTasks = s_Tasks[ezTaskPriority::SomeFrameMainThread].GetCount();

    ReprioritizeFrameTasks();
  }

  ExecuteSomeFrameTasks(uiSomeFrameTasks, s_fSmoothFrameMS);

  // Update the thread utilization
  {
    const ezTime tNow = ezTime::Now();
    static ezTime s_LastFrameUpdate = tNow;
    const ezTime tDiff = tNow - s_LastFrameUpdate;

    // prevent division by zero (inside ComputeThreadUtilization)
    if (tDiff > ezTime::Seconds(0.0))
    {
      s_LastFrameUpdate = tNow;

      for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
      {
        const ezUInt32 uiNumWorkers = s_iNumWorkerThreads[type];

        for (ezUInt32 t = 0; t < uiNumWorkers; ++t)
        {
          s_WorkerThreads[type][t]->UpdateThreadUtilization(tDiff);
        }
      }
    }
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
