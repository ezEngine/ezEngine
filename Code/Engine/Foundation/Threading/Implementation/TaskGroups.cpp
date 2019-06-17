#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>

ezTaskGroup::ezTaskGroup() = default;
ezTaskGroup::~ezTaskGroup() = default;

ezTaskGroupID ezTaskSystem::CreateTaskGroup(ezTaskPriority::Enum Priority, ezTaskGroup::OnTaskGroupFinished Callback)
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
  s_TaskGroups.PushBack(ezTaskGroup());
  s_TaskGroups[i].m_uiTaskGroupIndex = static_cast<ezUInt16>(i);

foundtaskgroup:

  s_TaskGroups[i].m_bInUse = true;
  s_TaskGroups[i].m_bStartedByUser = false;
  s_TaskGroups[i].m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  s_TaskGroups[i].m_Tasks.Clear();
  s_TaskGroups[i].m_DependsOn.Clear();
  s_TaskGroups[i].m_OthersDependingOnMe.Clear();
  s_TaskGroups[i].m_Priority = Priority;
  s_TaskGroups[i].m_OnFinishedCallback = Callback;

  ezTaskGroupID id;
  id.m_pTaskGroup = &s_TaskGroups[i];
  id.m_uiGroupCounter = s_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void ezTaskSystem::DebugCheckTaskGroup(ezTaskGroupID Group)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_LOCK(s_TaskSystemMutex);

  EZ_ASSERT_DEV(Group.m_pTaskGroup != nullptr, "TaskGroupID is invalid.");
  EZ_ASSERT_DEV(Group.m_pTaskGroup->m_uiGroupCounter == Group.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  EZ_ASSERT_DEV(!Group.m_pTaskGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  EZ_ASSERT_DEV(Group.m_pTaskGroup->m_iActiveDependencies == 0, "Invalid active dependenices");
#endif
}

void ezTaskSystem::AddTaskToGroup(ezTaskGroupID Group, ezTask* pTask)
{
  EZ_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  EZ_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  EZ_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  DebugCheckTaskGroup(Group);

  pTask->Reset();
  pTask->m_BelongsToGroup = Group;
  Group.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void ezTaskSystem::AddTaskGroupDependency(ezTaskGroupID Group, ezTaskGroupID DependsOn)
{
  EZ_ASSERT_DEBUG(DependsOn.IsValid(), "Invalid dependency");
  EZ_ASSERT_DEBUG(
    Group.m_pTaskGroup != DependsOn.m_pTaskGroup || Group.m_uiGroupCounter != DependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  DebugCheckTaskGroup(Group);

  Group.m_pTaskGroup->m_DependsOn.PushBack(DependsOn);
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

void ezTaskSystem::StartTaskGroup(ezTaskGroupID Group)
{
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == 0)
    SetWorkerThreadCount(-1, -1); // set the default number of threads, if none are started yet

  DebugCheckTaskGroup(Group);

  ezInt32 iActiveDependencies = 0;

  {
    EZ_LOCK(s_TaskSystemMutex);

    ezTaskGroup& tg = *Group.m_pTaskGroup;

    tg.m_bStartedByUser = true;

    for (ezUInt32 i = 0; i < tg.m_DependsOn.GetCount(); ++i)
    {
      // if the counters still match, the other task group has not yet been finished, and thus is a real dependency
      if (!IsTaskGroupFinished(tg.m_DependsOn[i]))
      {
        ezTaskGroup& Dependency = *tg.m_DependsOn[i].m_pTaskGroup;

        // add this task group to the list of dependencies, such that when that group finishes, this task group can get woken up
        Dependency.m_OthersDependingOnMe.PushBack(Group);

        // count how many other groups need to finish before this task group can be executed
        ++iActiveDependencies;
      }
    }

    if (iActiveDependencies != 0)
    {
      // apparently atomic integers are quite slow, so do not use them in the loop, where they are not yet needed
      tg.m_iActiveDependencies = iActiveDependencies;
    }
  }

  if (iActiveDependencies == 0)
  {
    ScheduleGroupTasks(Group.m_pTaskGroup);
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

void ezTaskSystem::ScheduleGroupTasks(ezTaskGroup* pGroup)
{
  if (pGroup->m_Tasks.IsEmpty())
  {
    pGroup->m_iRemainingTasks = 1;

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

    pGroup->m_iRemainingTasks = iRemainingTasks;


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
        s_Tasks[pGroup->m_Priority].PushBack(td);
      }
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
      const ezUInt32 uiNumSignals = ezMath::Min<ezUInt32>(iRemainingTasks, s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount());

      for (ezUInt32 i = 0; i < uiNumSignals; ++i)
      {
        s_TasksAvailableSignal[ezWorkerThreadType::ShortTasks].RaiseSignal();
      }
      break;
    }

    case ezTaskPriority::LongRunning:
    case ezTaskPriority::LongRunningHighPriority:
    {
      const ezUInt32 uiNumSignals = ezMath::Min<ezUInt32>(iRemainingTasks, s_WorkerThreads[ezWorkerThreadType::LongTasks].GetCount());

      for (ezUInt32 i = 0; i < uiNumSignals; ++i)
      {
        s_TasksAvailableSignal[ezWorkerThreadType::LongTasks].RaiseSignal();
      }
      break;
    }

    case ezTaskPriority::FileAccess:
    case ezTaskPriority::FileAccessHighPriority:
    {
      const ezUInt32 uiNumSignals = ezMath::Min<ezUInt32>(iRemainingTasks, s_WorkerThreads[ezWorkerThreadType::FileAccess].GetCount());

      for (ezUInt32 i = 0; i < uiNumSignals; ++i)
      {
        s_TasksAvailableSignal[ezWorkerThreadType::FileAccess].RaiseSignal();
      }
      break;
    }

    case ezTaskPriority::SomeFrameMainThread:
    case ezTaskPriority::ThisFrameMainThread:
    case ezTaskPriority::ENUM_COUNT:
      // nothing to do for these enum values
      break;
  }
}

void ezTaskSystem::DependencyHasFinished(ezTaskGroup* pGroup)
{
  // remove one dependency from the group
  if (pGroup->m_iActiveDependencies.Decrement() == 0)
  {
    // if there are no remaining dependencies, kick off all tasks in this group
    ScheduleGroupTasks(pGroup);
  }
}

void ezTaskSystem::WaitForGroup(ezTaskGroupID Group)
{
  if (ezTaskSystem::IsTaskGroupFinished(Group))
    return;

  // This function does basically the same as 'WaitForTask' only that it waits until ALL tasks of an entire group are finished
  // This function is less goal oriented, it does not try to pick out tasks that belong to the given group (at the moment)
  // It simply helps running tasks, until the given Group has been finished as well

  EZ_PROFILE_SCOPE("WaitForGroup");

  while (!ezTaskSystem::IsTaskGroupFinished(Group))
  {
    if (!HelpExecutingTasks())
    {
      // 'give up'
      ezThreadUtils::YieldTimeSlice();

      // if the system hangs / deadlocks, check the following:
      // is s_Tasks empty ? if so, no tasks are currently available to run
      // what are the dependencies of Group.m_pTaskGroup (DependsOn)
      // use the group counter member inside the ezTaskGroup to reference whether an ezTaskGroupID references the same group
      // or whether it has already been reused (ie. it is finished)
      // check the m_iActiveDependencies on an ezTaskgroup to see how many dependencies are really still waited for
      // check that all dependencies have indeed been started by the user, (m_bStartedByUser), if not a task group may be added as a
      // dependency, but forgotten to be launched, which can easily deadlock the system
      // also check the various callstacks, that a task may not be waiting in a circular fashion on another task, which is executed
      // on the same thread, but is also waiting for something else to finish. in that case you may need to handle dependencies and
      // task priorities better
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
      res = EZ_FAILURE;
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (OnTaskRunning == ezOnTaskRunning::WaitTillFinished && res == EZ_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (ezUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
      CancelTask(TasksCopy[task], ezOnTaskRunning::WaitTillFinished);
  }

  return res;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskGroups);
