#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

ezTask::ezTask(const char* szTaskName /*= nullptr*/)
{
  Reset();

  m_iRemainingRuns = 0;
  m_sTaskName = szTaskName;
}

void ezTask::Reset()
{
  m_iRemainingRuns = (int)ezMath::Max(1u, m_uiMultiplicity);
  m_bCancelExecution = false;
  m_bTaskIsScheduled = false;
}

void ezTask::SetTaskName(const char* szName)
{
  m_sTaskName = szName;
}

void ezTask::SetMultiplicity(ezUInt32 uiMultiplicity)
{
  EZ_ASSERT_DEV(!m_bTaskIsScheduled, "This task has already been scheduled to run, the multiplicity cannot be changed anymore.");

  m_uiMultiplicity = uiMultiplicity;
}

void ezTask::SetOnTaskFinished(OnTaskFinished Callback)
{
  m_OnTaskFinished = Callback;
}

void ezTask::Run(ezUInt32 uiInvocation)
{
  // actually this should not be possible to happen
  if (m_iRemainingRuns == 0 || m_bCancelExecution)
  {
    m_iRemainingRuns = 0;
    return;
  }

  {
    ezStringBuilder scopeName = m_sTaskName;

    if (m_uiMultiplicity > 0)
      scopeName.AppendFormat("-{}", uiInvocation);

    EZ_PROFILE_SCOPE(scopeName.GetData());

    if (m_uiMultiplicity > 0)
    {
      ExecuteWithMultiplicity(uiInvocation);
    }
    else
    {
      Execute();
    }
  }

  m_iRemainingRuns.Decrement();
}

void ezTaskSystem::TaskHasFinished(ezTask* pTask, ezTaskGroup* pGroup)
{
  // this might deallocate the task, make sure not to reference it later anymore
  if (pTask && pTask->m_OnTaskFinished.IsValid() && pTask->m_iRemainingRuns == 0)
  {
    pTask->m_OnTaskFinished(pTask);
  }

  if (pGroup->m_iRemainingTasks.Decrement() == 0)
  {
    // If this was the last task that had to be finished from this group, make sure all dependent groups are started

    // set this task group to be finished such that no one tries to append further dependencies
    pGroup->m_uiGroupCounter += 2;

    {
      EZ_LOCK(s_TaskSystemMutex);

      for (ezUInt32 dep = 0; dep < pGroup->m_OthersDependingOnMe.GetCount(); ++dep)
      {
        DependencyHasFinished(pGroup->m_OthersDependingOnMe[dep].m_pTaskGroup);
      }
    }

    if (pGroup->m_OnFinishedCallback.IsValid())
      pGroup->m_OnFinishedCallback();

    // set this task available for reuse
    pGroup->m_bInUse = false;
  }
}

ezTaskSystem::TaskData ezTaskSystem::GetNextTask(
  ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, ezTask* pPrioritizeThis)
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

  for (ezUInt32 i = FirstPriority; i <= (ezUInt32)LastPriority; ++i)
  {
    if (!s_Tasks[i].IsEmpty())
      goto foundany;
  }

  {
    TaskData td;
    td.m_pTask = nullptr;
    td.m_pBelongsToGroup = nullptr;
    return td;
  }


foundany:
  // we have detected that there MIGHT be work

  EZ_LOCK(s_TaskSystemMutex);

  // if there is a task that should be prioritized, check if it exists in any of the task lists
  if (pPrioritizeThis != nullptr)
  {
    // only search for the task in the lists that this thread is willing to work on
    // otherwise we might execute a main-thread task in a thread that is not the main thread
    for (ezUInt32 i = FirstPriority; i <= (ezUInt32)LastPriority; ++i)
    {
      auto it = s_Tasks[i].GetIterator();

      // just blindly search the entire list
      while (it.IsValid())
      {
        // if we find that task, return it
        // otherwise this whole search will do nothing and the default priority based
        // system will take over
        if (it->m_pTask == pPrioritizeThis)
        {
          TaskData td = *it;

          s_Tasks[i].Remove(it);
          return td;
        }

        ++it;
      }
    }
  }

  // go through all the task lists that this thread is willing to work on
  for (ezUInt32 i = FirstPriority; i <= (ezUInt32)LastPriority; ++i)
  {
    // if the list is not empty, just take the first task and execute that
    if (!s_Tasks[i].IsEmpty())
    {
      TaskData td = *s_Tasks[i].GetIterator();

      s_Tasks[i].Remove(s_Tasks[i].GetIterator());
      return td;
    }
  }

  {
    TaskData td;
    td.m_pTask = nullptr;
    td.m_pBelongsToGroup = nullptr;
    return td;
  }
}

bool ezTaskSystem::ExecuteTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, ezTask* pPrioritizeThis)
{
  ezTaskSystem::TaskData td = GetNextTask(FirstPriority, LastPriority, pPrioritizeThis);

  if (td.m_pTask == nullptr)
    return false;

  td.m_pTask->Run(td.m_uiInvocation);

  // notify the group, that a task is finished, which might trigger other tasks to be executed
  TaskHasFinished(td.m_pTask, td.m_pBelongsToGroup);

  return true;
}

void ezTaskSystem::WaitForTask(ezTask* pTask)
{
  if (pTask->IsTaskFinished())
    return;

  EZ_PROFILE_SCOPE("WaitForTask");

  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::LateNextFrame;

  // this specifies whether WaitForTask may fall back to processing standard tasks, when there is no more specific work available
  // in some cases we absolutely want to avoid that, since it can produce deadlocks
  // E.g. on the loading thread, if we are in the process of loading something and then we have to wait for something else,
  // we must not start that work on the loading thread, because once THAT task runs into something where it has to wait for something
  // to be loaded, we have a circular dependency on the thread itself and thus a deadlock
  bool bAllowDefaultWork = true;

  if (ezThreadUtils::IsMainThread())
  {
    // if this is the main thread, we need to execute the main-thread tasks
    // otherwise a dependency on which pTask is waiting, might not get fulfilled
    FirstPriority = ezTaskPriority::ThisFrameMainThread;
    LastPriority = ezTaskPriority::SomeFrameMainThread;

    /// \todo It is currently unclear whether bAllowDefaultWork should be false here as well (in which case the whole fall back mechanism
    /// could be removed)
    bAllowDefaultWork = false;
  }
  else if (IsLoadingThread())
  {
    FirstPriority = ezTaskPriority::FileAccessHighPriority;
    LastPriority = ezTaskPriority::FileAccess;
    bAllowDefaultWork = false;
  }
  else if (IsLongRunningThread())
  {
    FirstPriority = ezTaskPriority::LongRunningHighPriority;
    LastPriority = ezTaskPriority::LongRunning;
    bAllowDefaultWork = false;
  }

  while (!pTask->IsTaskFinished())
  {
    // we only execute short tasks here, because you should never WAIT for a long running task
    // and a short task should never have a dependency on a long running task either
    // so we assume that only short running tasks need to be executed to fulfill the task's dependencies
    // Since there are threads to deal with long running tasks in parallel, even if we were waiting for such
    // a task, it will get finished at some point
    if (!ExecuteTask(FirstPriority, LastPriority, pTask))
    {
      if (!pTask->IsTaskFinished())
      {
        // if there was nothing for us to do, that probably means that the task is either currently being processed by some other thread
        // or it is in a priority list that we did not want to work on (maybe because we are on the main thread)
        // in this case try it again with non-main-thread tasks

        // if bAllowDefaultWork is false, we just always yield here

        if (!bAllowDefaultWork || !ExecuteTask(ezTaskPriority::EarlyThisFrame, ezTaskPriority::LateNextFrame, pTask))
        {
          // if there is STILL nothing for us to do, it might be a long running task OR it is already being processed
          // we won't fall back to processing long running tasks, because that might stall the application
          // instead we assume the task (or any dependency) is currently processed by another thread
          // and to prevent a busy loop, we just give up our time-slice and try again later
          ezThreadUtils::YieldTimeSlice();
        }
      }
    }
  }
}

ezResult ezTaskSystem::CancelTask(ezTask* pTask, ezOnTaskRunning::Enum OnTaskRunning)
{
  if (pTask->IsTaskFinished())
    return EZ_SUCCESS;

  EZ_PROFILE_SCOPE("CancelTask");

  EZ_ASSERT_DEV(pTask->m_BelongsToGroup.m_pTaskGroup->m_uiGroupCounter == pTask->m_BelongsToGroup.m_uiGroupCounter,
    "The task to be removed is in an invalid group.");

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
    WaitForTask(pTask);

  return EZ_FAILURE;
}

bool ezTaskSystem::HelpExecutingTasks()
{
  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::LateNextFrame;

  if (ezThreadUtils::IsMainThread())
  {
    // if this is the main thread, we need to execute the main-thread tasks
    FirstPriority = ezTaskPriority::ThisFrameMainThread;
    LastPriority = ezTaskPriority::SomeFrameMainThread;
  }

  return ExecuteTask(FirstPriority, LastPriority);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Tasks);
