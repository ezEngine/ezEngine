#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>

ezTaskGroupID ezTaskSystem::CreateTaskGroup(ezTaskPriority::Enum Priority, ezOnTaskGroupFinishedCallback callback)
{
  EZ_LOCK(s_TaskSystemMutex);

  ezUInt32 i = 0;

  // this search could be speed up with a stack of free groups
  for (; i < s_TaskGroups.GetCount(); ++i)
  {
    if (!s_TaskGroups[i].m_bInUse)
      goto foundtaskgroup;
  }

  // no free group found, create a new one
  s_TaskGroups.ExpandAndGetRef();
  s_TaskGroups[i].m_uiTaskGroupIndex = static_cast<ezUInt16>(i);

foundtaskgroup:

  s_TaskGroups[i].Reuse(Priority, callback);

  ezTaskGroupID id;
  id.m_pTaskGroup = &s_TaskGroups[i];
  id.m_uiGroupCounter = s_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void ezTaskSystem::AddTaskToGroup(ezTaskGroupID groupID, ezTask* pTask)
{
  EZ_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  EZ_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  EZ_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  ezTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  pTask->Reset();
  pTask->m_BelongsToGroup = groupID;
  groupID.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void ezTaskSystem::AddTaskGroupDependency(ezTaskGroupID groupID, ezTaskGroupID DependsOn)
{
  EZ_ASSERT_DEBUG(DependsOn.IsValid(), "Invalid dependency");
  EZ_ASSERT_DEBUG(groupID.m_pTaskGroup != DependsOn.m_pTaskGroup || groupID.m_uiGroupCounter != DependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  ezTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  groupID.m_pTaskGroup->m_DependsOnGroups.PushBack(DependsOn);
}

void ezTaskSystem::AddTaskGroupDependencyBatch(ezArrayPtr<const ezTaskGroupDependency> batch)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_LOCK(s_TaskSystemMutex);
#endif

  for (const ezTaskGroupDependency& dep : batch)
  {
    AddTaskGroupDependency(dep.m_TaskGroup, dep.m_DependsOn);
  }
}

void ezTaskSystem::StartTaskGroup(ezTaskGroupID groupID)
{
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == 0)
    SetWorkerThreadCount(-1, -1); // set the default number of threads, if none are started yet

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

bool ezTaskSystem::IsTaskGroupFinished(ezTaskGroupID Group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return (Group.m_pTaskGroup == nullptr) || (Group.m_pTaskGroup->m_uiGroupCounter != Group.m_uiGroupCounter);
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
          s_Tasks[pGroup->m_Priority].PushFront(td);
        else
          s_Tasks[pGroup->m_Priority].PushBack(td);
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

void ezTaskSystem::WaitForGroup(ezTaskGroupID Group)
{
  EZ_PROFILE_SCOPE("WaitForGroup");

  EZ_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != ezWorkerThreadType::MainThread;

  while (!ezTaskSystem::IsTaskGroupFinished(Group))
  {
    if (!HelpExecutingTasks(Group))
    {
      if (bAllowSleep)
      {
        s_BlockedWorkerThreads[ThreadTaskType].Increment();
        const ezWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == ezWorkerThreadType::Unknown) ? ezWorkerThreadType::ShortTasks : ThreadTaskType;

        WakeUpThreads(typeToWakeUp, 1);

        Group.m_pTaskGroup->WaitForFinish(Group);

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

ezResult ezTaskSystem::CancelGroup(ezTaskGroupID Group, ezOnTaskRunning::Enum OnTaskRunning)
{
  if (ezTaskSystem::IsTaskGroupFinished(Group))
    return EZ_SUCCESS;

  EZ_PROFILE_SCOPE("CancelGroup");

  EZ_LOCK(s_TaskSystemMutex);

  ezResult res = EZ_SUCCESS;

  ezHybridArray<ezTask*, 16> TasksCopy = Group.m_pTaskGroup->m_Tasks;

  // first cancel ALL the tasks in the group, without waiting for anything
  for (ezUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
  {
    if (CancelTask(TasksCopy[task], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_FAILURE)
    {
      res = EZ_FAILURE;
    }
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (OnTaskRunning == ezOnTaskRunning::WaitTillFinished && res == EZ_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (ezUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
    {
      CancelTask(TasksCopy[task], ezOnTaskRunning::WaitTillFinished);
    }
  }

  return res;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskGroups);
