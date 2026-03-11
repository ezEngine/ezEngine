#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>


ezTaskGroupID ezTaskSystem::CreateTaskGroup(ezTaskPriority::Enum priority, ezOnTaskGroupFinishedCallback callback)
{
  EZ_LOCK(s_TaskSystemMutex);

  ezUInt32 i = 0;

  // this search could be speed up with a stack of free groups
  for (; i < s_pState->m_TaskGroups.GetCount(); ++i)
  {
    if (!s_pState->m_TaskGroups[i].m_bInUse)
    {
      goto foundtaskgroup;
    }
  }

  // no free group found, create a new one
  s_pState->m_TaskGroups.ExpandAndGetRef();
  s_pState->m_TaskGroups[i].m_uiTaskGroupIndex = static_cast<ezUInt16>(i);

foundtaskgroup:

  s_pState->m_TaskGroups[i].Reuse(priority, callback);

  ezTaskGroupID id;
  id.m_pTaskGroup = &s_pState->m_TaskGroups[i];
  id.m_uiGroupCounter = s_pState->m_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void ezTaskSystem::AddTaskToGroup(ezTaskGroupID groupID, const ezSharedPtr<ezTask>& pTask)
{
  EZ_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  EZ_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  EZ_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  ezTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  pTask->Reset();
  pTask->m_BelongsToGroup = groupID;
  groupID.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void ezTaskSystem::AddTaskGroupDependency(ezTaskGroupID groupID, ezTaskGroupID dependsOn)
{
  EZ_ASSERT_DEBUG(dependsOn.IsValid(), "Invalid dependency");
  EZ_ASSERT_DEBUG(groupID.m_pTaskGroup != dependsOn.m_pTaskGroup || groupID.m_uiGroupCounter != dependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  ezTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  groupID.m_pTaskGroup->m_DependsOnGroups.PushBack(dependsOn);
}

void ezTaskSystem::AddTaskGroupDependencyBatch(ezArrayPtr<const ezTaskGroupDependency> batch)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // lock here once to reduce the overhead of ezTaskGroup::DebugCheckTaskGroup inside AddTaskGroupDependency
  EZ_LOCK(s_TaskSystemMutex);
#endif

  for (const ezTaskGroupDependency& dep : batch)
  {
    AddTaskGroupDependency(dep.m_TaskGroup, dep.m_DependsOn);
  }
}

void ezTaskSystem::StartTaskGroup(ezTaskGroupID groupID)
{
  EZ_ASSERT_DEV(s_pThreadState->m_Workers[ezWorkerThreadType::ShortTasks].GetCount() > 0, "No worker threads started.");

  ezTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  ezInt32 iActiveDependencies = 0;

  {
    EZ_LOCK(s_TaskSystemMutex);

    ezTaskGroup& tg = *groupID.m_pTaskGroup;

    tg.m_bStartedByUser = true;

    for (ezUInt32 i = 0; i < tg.m_DependsOnGroups.GetCount(); ++i)
    {
      if (!IsTaskGroupFinished(tg.m_DependsOnGroups[i]))
      {
        ezTaskGroup& Dependency = *tg.m_DependsOnGroups[i].m_pTaskGroup;

        // add this task group to the list of dependencies, such that when that group finishes, this task group can get woken up
        Dependency.m_OthersDependingOnMe.PushBack(groupID);

        // count how many other groups need to finish before this task group can be executed
        ++iActiveDependencies;
      }
    }

    if (iActiveDependencies != 0)
    {
      // atomic integers are quite slow, so do not use them in the loop, where they are not yet needed
      tg.m_iNumActiveDependencies = iActiveDependencies;
    }
  }

  if (iActiveDependencies == 0)
  {
    ScheduleGroupTasks(groupID.m_pTaskGroup, false);
  }
}

void ezTaskSystem::StartTaskGroupBatch(ezArrayPtr<const ezTaskGroupID> batch)
{
  EZ_LOCK(s_TaskSystemMutex);

  for (const ezTaskGroupID& group : batch)
  {
    StartTaskGroup(group);
  }
}

bool ezTaskSystem::IsTaskGroupFinished(ezTaskGroupID group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return (group.m_pTaskGroup == nullptr) || (group.m_pTaskGroup->m_uiGroupCounter != group.m_uiGroupCounter);
}

void ezTaskSystem::ScheduleGroupTasks(ezTaskGroup* pGroup, bool bHighPriority)
{
  if (pGroup->m_Tasks.IsEmpty())
  {
    pGroup->m_iNumRemainingTasks = 1;

    // "finish" one task -> will finish the task group and kick off dependent groups
    TaskHasFinished(nullptr, pGroup);
    return;
  }

  ezInt32 iRemainingTasks = 0;

  // add all the tasks to the task list, so that they will be processed
  {
    EZ_LOCK(s_TaskSystemMutex);


    // store how many tasks from this groups still need to be processed

    for (auto pTask : pGroup->m_Tasks)
    {
      iRemainingTasks += ezMath::Max(1u, pTask->m_uiMultiplicity);
      pTask->m_iRemainingRuns = ezMath::Max(1u, pTask->m_uiMultiplicity);
    }

    pGroup->m_iNumRemainingTasks = iRemainingTasks;


    for (ezUInt32 task = 0; task < pGroup->m_Tasks.GetCount(); ++task)
    {
      auto& pTask = pGroup->m_Tasks[task];

      for (ezUInt32 mult = 0; mult < ezMath::Max(1u, pTask->m_uiMultiplicity); ++mult)
      {
        TaskData td;
        td.m_pBelongsToGroup = pGroup;
        td.m_pTask = pTask;
        td.m_pTask->m_bTaskIsScheduled = true;
        td.m_uiInvocation = mult;

        if (bHighPriority)
          s_pState->m_Tasks[pGroup->m_Priority].PushFront(td);
        else
          s_pState->m_Tasks[pGroup->m_Priority].PushBack(td);
      }
    }

    // send the proper thread signal, to make sure one of the correct worker threads is awake
    switch (pGroup->m_Priority)
    {
      case ezTaskPriority::EarlyThisFrame:
      case ezTaskPriority::ThisFrame:
      case ezTaskPriority::LateThisFrame:
      case ezTaskPriority::EarlyNextFrame:
      case ezTaskPriority::NextFrame:
      case ezTaskPriority::LateNextFrame:
      case ezTaskPriority::In2Frames:
      case ezTaskPriority::In3Frames:
      case ezTaskPriority::In4Frames:
      case ezTaskPriority::In5Frames:
      case ezTaskPriority::In6Frames:
      case ezTaskPriority::In7Frames:
      case ezTaskPriority::In8Frames:
      case ezTaskPriority::In9Frames:
      {
        WakeUpThreads(ezWorkerThreadType::ShortTasks, iRemainingTasks);
        break;
      }

      case ezTaskPriority::LongRunning:
      case ezTaskPriority::LongRunningHighPriority:
      {
        WakeUpThreads(ezWorkerThreadType::LongTasks, iRemainingTasks);
        break;
      }

      case ezTaskPriority::FileAccess:
      case ezTaskPriority::FileAccessHighPriority:
      {
        WakeUpThreads(ezWorkerThreadType::FileAccess, iRemainingTasks);
        break;
      }

      case ezTaskPriority::SomeFrameMainThread:
      case ezTaskPriority::ThisFrameMainThread:
      case ezTaskPriority::ENUM_COUNT:
        // nothing to do for these enum values
        break;
    }
  }
}

void ezTaskSystem::DependencyHasFinished(ezTaskGroup* pGroup)
{
  // remove one dependency from the group
  if (pGroup->m_iNumActiveDependencies.Decrement() == 0)
  {
    // if there are no remaining dependencies, kick off all tasks in this group
    ScheduleGroupTasks(pGroup, true);
  }
}

ezResult ezTaskSystem::CancelGroup(ezTaskGroupID group, ezOnTaskRunning::Enum onTaskRunning)
{
  if (ezTaskSystem::IsTaskGroupFinished(group))
    return EZ_SUCCESS;

  EZ_PROFILE_SCOPE("CancelGroup");

  EZ_LOCK(s_TaskSystemMutex);

  ezResult res = EZ_SUCCESS;

  auto TasksCopy = group.m_pTaskGroup->m_Tasks;

  // first cancel ALL the tasks in the group, without waiting for anything
  for (ezUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
  {
    if (CancelTask(TasksCopy[task], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_FAILURE)
    {
      res = EZ_FAILURE;
    }
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (onTaskRunning == ezOnTaskRunning::WaitTillFinished && res == EZ_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (ezUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
    {
      CancelTask(TasksCopy[task], ezOnTaskRunning::WaitTillFinished).IgnoreResult();
    }
  }

  return res;
}

void ezTaskSystem::WaitForGroup(ezTaskGroupID group)
{
  EZ_PROFILE_SCOPE("ezTaskSystem::WaitForGroup");

  EZ_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != ezWorkerThreadType::MainThread;

  while (!ezTaskSystem::IsTaskGroupFinished(group))
  {
    if (!HelpExecutingTasks(group))
    {
      if (bAllowSleep)
      {
        const ezWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == ezWorkerThreadType::Unknown) ? ezWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          EZ_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)ezTaskWorkerState::Blocked) == (int)ezTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        group.m_pTaskGroup->WaitForFinish(group);

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          EZ_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)ezTaskWorkerState::Active) == (int)ezTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        ezThreadUtils::YieldTimeSlice();
      }
    }
  }
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
        const ezWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == ezWorkerThreadType::Unknown) ? ezWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          EZ_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)ezTaskWorkerState::Blocked) == (int)ezTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        while (!condition())
        {
          // TODO: busy loop for now
          ezThreadUtils::YieldTimeSlice();
        }

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          EZ_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)ezTaskWorkerState::Active) == (int)ezTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        ezThreadUtils::YieldTimeSlice();
      }
    }
  }
}
