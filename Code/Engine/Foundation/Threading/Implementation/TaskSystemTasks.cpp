#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>

ezTaskGroupID ezTaskSystem::StartSingleTask(const ezSharedPtr<ezTask>& pTask, ezTaskPriority::Enum Priority, ezTaskGroupID Dependency, ezOnTaskGroupFinishedCallback callback /*= ezOnTaskGroupFinishedCallback()*/)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority, callback);
  AddTaskGroupDependency(Group, Dependency);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

ezTaskGroupID ezTaskSystem::StartSingleTask(const ezSharedPtr<ezTask>& pTask, ezTaskPriority::Enum Priority, ezOnTaskGroupFinishedCallback callback /*= ezOnTaskGroupFinishedCallback()*/)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority, callback);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

void ezTaskSystem::TaskHasFinished(const ezSharedPtr<ezTask>& pTask, ezTaskGroup* pGroup)
{
  if (pTask && pTask->m_OnTaskFinished.IsValid() && pTask->m_iRemainingRuns == 0)
  {
    pTask->m_OnTaskFinished(pTask);
  }

  if (pGroup->m_iNumRemainingTasks.Decrement() == 0)
  {
    // If this was the last task that had to be finished from this group, make sure all dependent groups are started

    ezUInt32 groupCounter = 0;
    {
      // see ezTaskGroup::WaitForFinish() for why we need this lock here
      // without it, there would be a race condition between these two places, reading and writing m_uiGroupCounter and waiting/signaling m_CondVarGroupFinished
      EZ_LOCK(pGroup->m_CondVarGroupFinished);

      groupCounter = pGroup->m_uiGroupCounter;

      // set this task group to be finished such that no one tries to append further dependencies
      pGroup->m_uiGroupCounter += 2;
    }

    // wake up all threads that are waiting for this group
    pGroup->m_CondVarGroupFinished.SignalAll();

    {
      EZ_LOCK(s_TaskSystemMutex);

      for (ezUInt32 dep = 0; dep < pGroup->m_OthersDependingOnMe.GetCount(); ++dep)
      {
        DependencyHasFinished(pGroup->m_OthersDependingOnMe[dep].m_pTaskGroup);
      }

      // unless an outside reference is held onto a task, this will deallocate the tasks
      pGroup->m_Tasks.Clear();
    }

    if (pGroup->m_OnFinishedCallback.IsValid())
    {
      ezTaskGroupID id;
      id.m_pTaskGroup = pGroup;
      id.m_uiGroupCounter = groupCounter;
      pGroup->m_OnFinishedCallback(id);
    }

    // set this task available for reuse
    pGroup->m_bInUse = false;
  }
}

ezTaskSystem::TaskData ezTaskSystem::GetNextTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicInteger32* pWorkerState)
{
  // this is the central function that selects tasks for the worker threads to work on

  EZ_ASSERT_DEV(FirstPriority >= ezTaskPriority::EarlyThisFrame && LastPriority < ezTaskPriority::ENUM_COUNT, "Priority Range is invalid: {0} to {1}", FirstPriority, LastPriority);

  EZ_LOCK(s_TaskSystemMutex);

  // go through all the task lists that this thread is willing to work on
  for (ezUInt32 prio = FirstPriority; prio <= (ezUInt32)LastPriority; ++prio)
  {
    for (auto it = s_State->m_Tasks[prio].GetIterator(); it.IsValid(); ++it)
    {
      if (!bOnlyTasksThatNeverWait || (it->m_pTask->m_NestingMode == ezTaskNesting::Never) || it->m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup)
      {
        TaskData td = *it;

        s_State->m_Tasks[prio].Remove(it);
        return td;
      }
    }
  }

  if (pWorkerState)
  {
    EZ_VERIFY(pWorkerState->Set((int)ezTaskWorkerState::Idle) == (int)ezTaskWorkerState::Active, "Corrupt Worker State");
  }

  return TaskData();
}

bool ezTaskSystem::ExecuteTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicInteger32* pWorkerState)
{
  //const ezWorkerThreadType::Enum workerType = (tl_TaskWorkerInfo.m_WorkerType == ezWorkerThreadType::Unknown) ? ezWorkerThreadType::ShortTasks : tl_TaskWorkerInfo.m_WorkerType;

  ezTaskSystem::TaskData td = GetNextTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, pWorkerState);

  if (td.m_pTask == nullptr)
    return false;

  if (bOnlyTasksThatNeverWait && td.m_pTask->m_NestingMode != ezTaskNesting::Never)
  {
    EZ_ASSERT_DEV(td.m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup, "");
  }

  tl_TaskWorkerInfo.m_bAllowNestedTasks = td.m_pTask->m_NestingMode != ezTaskNesting::Never;
  tl_TaskWorkerInfo.m_szTaskName = td.m_pTask->m_sTaskName;
  td.m_pTask->Run(td.m_uiInvocation);
  tl_TaskWorkerInfo.m_bAllowNestedTasks = true;
  tl_TaskWorkerInfo.m_szTaskName = nullptr;

  // notify the group, that a task is finished, which might trigger other tasks to be executed
  TaskHasFinished(td.m_pTask, td.m_pBelongsToGroup);

  return true;
}


ezResult ezTaskSystem::CancelTask(const ezSharedPtr<ezTask>& pTask, ezOnTaskRunning::Enum OnTaskRunning)
{
  if (pTask->IsTaskFinished())
    return EZ_SUCCESS;

  // pTask may actually finish between here and the lock below
  // in that case we will return failure, as in we had to 'wait' for a task,
  // but it will be handled correctly

  EZ_PROFILE_SCOPE("CancelTask");

  // we set the cancel flag, to make sure that tasks that support canceling will terminate asap
  pTask->m_bCancelExecution = true;

  {
    EZ_LOCK(s_TaskSystemMutex);

    // if the task is still in the queue of its group, it had not yet been scheduled
    if (!pTask->m_bTaskIsScheduled && pTask->m_BelongsToGroup.m_pTaskGroup->m_Tasks.RemoveAndSwap(pTask))
    {
      // we set the task to finished, even though it was not executed
      pTask->m_iRemainingRuns = 0;
      return EZ_SUCCESS;
    }

    // check if the task has already been scheduled for execution
    // if so, remove it from the work queue
    {
      for (ezUInt32 i = 0; i < ezTaskPriority::ENUM_COUNT; ++i)
      {
        auto it = s_State->m_Tasks[i].GetIterator();

        while (it.IsValid())
        {
          if (it->m_pTask == pTask)
          {
            s_State->m_Tasks[i].Remove(it);

            // we set the task to finished, even though it was not executed
            pTask->m_iRemainingRuns = 0;

            // tell the system that one task of that group is 'finished', to ensure its dependencies will get scheduled
            TaskHasFinished(it->m_pTask, it->m_pBelongsToGroup);
            return EZ_SUCCESS;
          }

          ++it;
        }
      }
    }
  }

  // if we made it here, the task was already running
  // thus we just wait for it to finish

  if (OnTaskRunning == ezOnTaskRunning::WaitTillFinished)
  {
    WaitForCondition([pTask]() { return pTask->IsTaskFinished(); });
  }

  return EZ_FAILURE;
}


bool ezTaskSystem::HelpExecutingTasks(const ezTaskGroupID& WaitingForGroup)
{
  const bool bOnlyTasksThatNeverWait = tl_TaskWorkerInfo.m_WorkerType != ezWorkerThreadType::MainThread;

  ezTaskPriority::Enum FirstPriority;
  ezTaskPriority::Enum LastPriority;
  DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  return ExecuteTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, nullptr);
}

void ezTaskSystem::ReprioritizeFrameTasks()
{
  // There should usually be no 'this frame tasks' left at this time
  // however, while we waited to enter the lock, such tasks might have appeared
  // In this case we move them into the highest-priority 'this frame' queue, to ensure they will be executed asap
  for (ezUInt32 i = (ezUInt32)ezTaskPriority::ThisFrame; i <= (ezUInt32)ezTaskPriority::LateThisFrame; ++i)
  {
    auto it = s_State->m_Tasks[i].GetIterator();

    // move all 'this frame' tasks into the 'early this frame' queue
    while (it.IsValid())
    {
      s_State->m_Tasks[ezTaskPriority::EarlyThisFrame].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_State->m_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::EarlyNextFrame; i <= (ezUInt32)ezTaskPriority::LateNextFrame; ++i)
  {
    auto it = s_State->m_Tasks[i].GetIterator();

    // move all 'next frame' tasks into the 'this frame' queues
    while (it.IsValid())
    {
      s_State->m_Tasks[i - 3].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_State->m_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::In2Frames; i <= (ezUInt32)ezTaskPriority::In9Frames; ++i)
  {
    auto it = s_State->m_Tasks[i].GetIterator();

    // move all 'in N frames' tasks into the 'in N-1 frames' queues
    // moves 'In2Frames' into 'LateNextFrame'
    while (it.IsValid())
    {
      s_State->m_Tasks[i - 1].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_State->m_Tasks[i].Clear();
  }
}

void ezTaskSystem::ExecuteSomeFrameTasks(ezUInt32 uiSomeFrameTasks, ezTime smoothFrameTime)
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

  static ezTime s_FrameTimeThreshold = smoothFrameTime;

  static ezTime s_LastFrame; // initializes to zero -> very large frame time difference at first

  ezTime CurTime = ezTime::Now();
  ezTime LastTime = s_LastFrame;
  s_LastFrame = CurTime;

  // as long as we have a smooth frame rate, execute as many of these tasks, as possible
  while ((uiSomeFrameTasks > 0) && (CurTime - LastTime < smoothFrameTime))
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_FrameTimeThreshold = smoothFrameTime;

    if (!ExecuteTask(ezTaskPriority::SomeFrameMainThread, ezTaskPriority::SomeFrameMainThread, false, ezTaskGroupID(), nullptr))
      return; // nothing left to do

    CurTime = ezTime::Now();
    --uiSomeFrameTasks;
  }

  // nothing left to do
  if (uiSomeFrameTasks == 0)
    return;

  if (CurTime - LastTime < s_FrameTimeThreshold)
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_FrameTimeThreshold = smoothFrameTime;

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
    s_FrameTimeThreshold += ezTime::Milliseconds(5.0);

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
    uiSomeFrameTasks = s_State->m_Tasks[ezTaskPriority::SomeFrameMainThread].GetCount();

    ReprioritizeFrameTasks();
  }

  ExecuteSomeFrameTasks(uiSomeFrameTasks, s_State->m_TargetFrameTime);

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
        const ezUInt32 uiNumWorkers = s_ThreadState->m_iAllocatedWorkers[type];

        for (ezUInt32 t = 0; t < uiNumWorkers; ++t)
        {
          s_ThreadState->m_Workers[type][t]->UpdateThreadUtilization(tDiff);
        }
      }
    }
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemTasks);
