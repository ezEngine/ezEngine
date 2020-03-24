#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

void ezTaskSystem::TaskHasFinished(ezTask* pTask, ezTaskGroup* pGroup)
{
  // this might deallocate the task, make sure not to reference it later anymore
  if (pTask && pTask->m_OnTaskFinished.IsValid() && pTask->m_iRemainingRuns == 0)
  {
    pTask->m_OnTaskFinished(pTask);
  }

  if (pGroup->m_iNumRemainingTasks.Decrement() == 0)
  {
    // If this was the last task that had to be finished from this group, make sure all dependent groups are started

    {
      // see ezTaskGroup::WaitForFinish() for why we need this lock here
      // without it, there would be a race condition between these two places, reading and writing m_uiGroupCounter and waiting/signaling m_CondVarGroupFinished
      EZ_LOCK(pGroup->m_CondVarGroupFinished);

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
    }

    if (pGroup->m_OnFinishedCallback.IsValid())
    {
      pGroup->m_OnFinishedCallback();
    }

    // set this task available for reuse
    pGroup->m_bInUse = false;
  }
}

ezTaskSystem::TaskData ezTaskSystem::GetNextTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicBool* pIsIdleNow)
{
  // this is the central function that selects tasks for the worker threads to work on

  // do a quick and dirty check, whether we would find any work (without a lock)
  // if nothing is found, the thread can go to sleep, without entering the mutex

  // note: There is NO race condition here. Even if this loop would miss a work item, because it is added after it looked at the
  // corresponding queue, it will not be a problem. The function will return with 'no work' for the thread,  the thread will try to go to
  // sleep, but the thread-signal will be signaled already and thus the thread will loop again, call 'GetNextTask' a second time and THEN
  // detect the new work item

  EZ_ASSERT_DEV(FirstPriority >= ezTaskPriority::EarlyThisFrame && LastPriority < ezTaskPriority::ENUM_COUNT,
    "Priority Range is invalid: {0} to {1}", FirstPriority, LastPriority);

  // TODO: early out with pIsIdleNow = true, if too many threads are active

  for (ezUInt32 i = FirstPriority; i <= (ezUInt32)LastPriority; ++i)
  {
    if (!s_Tasks[i].IsEmpty())
      goto foundany;
  }

  if (pIsIdleNow)
  {
    EZ_VERIFY(pIsIdleNow->Set(true) == false, "Corrupt Idle State");
  }

  return TaskData();

foundany:
  // we have detected that there MIGHT be work

  EZ_LOCK(s_TaskSystemMutex);

  // go through all the task lists that this thread is willing to work on
  for (ezUInt32 prio = FirstPriority; prio <= (ezUInt32)LastPriority; ++prio)
  {
    for (auto it = s_Tasks[prio].GetIterator(); it.IsValid(); ++it)
    {
      if (!bOnlyTasksThatNeverWait || (it->m_pTask->m_NestingMode == ezTaskNesting::Never) || it->m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup)
      {
        TaskData td = *it;

        s_Tasks[prio].Remove(it);
        return td;
      }
    }
  }

  if (pIsIdleNow)
  {
    EZ_VERIFY(pIsIdleNow->Set(true) == false, "Corrupt Idle State");
  }

  return TaskData();
}

bool ezTaskSystem::ExecuteTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicBool* pIsIdleNow)
{
  ezTaskSystem::TaskData td = GetNextTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, pIsIdleNow);

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

ezResult ezTaskSystem::CancelTask(ezTask* pTask, ezOnTaskRunning::Enum OnTaskRunning)
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
        auto it = s_Tasks[i].GetIterator();

        while (it.IsValid())
        {
          if (it->m_pTask == pTask)
          {
            s_Tasks[i].Remove(it);

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

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Tasks);
