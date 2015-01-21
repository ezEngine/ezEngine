#include <Foundation/PCH.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Math.h>

ezTaskGroupID::ezTaskGroupID()
{
  m_pTaskGroup = nullptr;
  m_uiGroupCounter = 0;
}

ezTaskGroup::ezTaskGroup()
{
  m_bInUse = false;
  m_bIsActive = false;
  m_uiGroupCounter = 1;
  m_Priority = ezTaskPriority::ThisFrame;
  m_OnFinishedCallback = nullptr;
  m_pCallbackPassThrough = nullptr;
}

ezTaskGroupID ezTaskSystem::CreateTaskGroup(ezTaskPriority::Enum Priority, ezTaskGroup::OnTaskGroupFinished Callback, void* pPassThrough)
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

foundtaskgroup:

  s_TaskGroups[i].m_bInUse = true;
  s_TaskGroups[i].m_bIsActive = false;
  s_TaskGroups[i].m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  s_TaskGroups[i].m_Tasks.Clear();
  s_TaskGroups[i].m_DependsOn.Clear();
  s_TaskGroups[i].m_OthersDependingOnMe.Clear();
  s_TaskGroups[i].m_Priority = Priority;
  s_TaskGroups[i].m_OnFinishedCallback = Callback;
  s_TaskGroups[i].m_pCallbackPassThrough = pPassThrough;

  ezTaskGroupID id;
  id.m_pTaskGroup = &s_TaskGroups[i];
  id.m_uiGroupCounter = s_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void ezTaskSystem::DebugCheckTaskGroup(ezTaskGroupID Group)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_LOCK(s_TaskSystemMutex);

  EZ_ASSERT_DEV(Group.m_pTaskGroup != nullptr, "TaskGroupID is invalid.");
  EZ_ASSERT_DEV(Group.m_pTaskGroup->m_uiGroupCounter == Group.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  EZ_ASSERT_DEV(!Group.m_pTaskGroup->m_bIsActive, "The given TaskGroupID is already started, you cannot modify it anymore.");
#endif
}

void ezTaskSystem::AddTaskToGroup(ezTaskGroupID Group, ezTask* pTask)
{
  EZ_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  EZ_ASSERT_DEV(pTask->IsTaskFinished(), "The task that is not finished! Cannot reuse a task before it is done.");

  DebugCheckTaskGroup(Group);

  pTask->Reset();
  pTask->m_BelongsToGroup = Group;
  Group.m_pTaskGroup->m_Tasks.PushBack(pTask);

  pTask->CreateProfilingID();
}

void ezTaskSystem::AddTaskGroupDependency(ezTaskGroupID Group, ezTaskGroupID DependsOn)
{
  DebugCheckTaskGroup(Group);

  Group.m_pTaskGroup->m_DependsOn.PushBack(DependsOn);
}

void ezTaskSystem::StartTaskGroup(ezTaskGroupID Group)
{
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == 0)
    SetWorkThreadCount(-1, -1); // set the default number of threads, if none are started yet

  DebugCheckTaskGroup(Group);

  ezInt32 iActiveDependencies = 0;

  {
    EZ_LOCK(s_TaskSystemMutex);

    ezTaskGroup& tg = *Group.m_pTaskGroup;

    tg.m_bIsActive = true;

    for (ezUInt32 i = 0; i < tg.m_DependsOn.GetCount(); ++i)
    {
      ezTaskGroup& Dependency = *tg.m_DependsOn[i].m_pTaskGroup;

      // if the counters still match, the other task group has not yet been finished, and thus is a real dependency
      if (!IsTaskGroupFinished(tg.m_DependsOn[i]))
      {
        // add this task group to that the list of the dependency, such that when that group finishes, this task group can get woken up
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

bool ezTaskSystem::IsTaskGroupFinished(ezTaskGroupID Group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return Group.m_pTaskGroup->m_uiGroupCounter != Group.m_uiGroupCounter;
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

  // add all the tasks to the task list, so that they will be processed
  {
    EZ_LOCK(s_TaskSystemMutex);

    // store how many tasks from this groups still need to be processed
    pGroup->m_iRemainingTasks = pGroup->m_Tasks.GetCount();

    TaskData td;
    td.m_pBelongsToGroup = pGroup;

    for (ezUInt32 task = 0; task < pGroup->m_Tasks.GetCount(); ++task)
    {
      td.m_pTask = pGroup->m_Tasks[task];
      td.m_pTask->m_bTaskIsScheduled = true;
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
    s_TasksAvailableSignal[ezWorkerThreadType::ShortTasks].RaiseSignal();
    break;

  case ezTaskPriority::LongRunning:
  case ezTaskPriority::LongRunningHighPriority:
    s_TasksAvailableSignal[ezWorkerThreadType::LongTasks].RaiseSignal();
    break;

  case ezTaskPriority::FileAccess:
  case ezTaskPriority::FileAccessHighPriority:
    s_TasksAvailableSignal[ezWorkerThreadType::FileAccess].RaiseSignal();
    break;

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

  EZ_PROFILE(s_ProfileWaitForGroup);

  const bool bIsMainThread = ezThreadUtils::IsMainThread();

  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::LateNextFrame;

  if (bIsMainThread)
  {
    // if this is the main thread, we need to execute the main-thread tasks
    // otherwise a dependency on which Group is waiting, might not get fulfilled
    FirstPriority = ezTaskPriority::ThisFrameMainThread;
    LastPriority = ezTaskPriority::SomeFrameMainThread;
  }

  while (!ezTaskSystem::IsTaskGroupFinished(Group))
  {
    // first try the main-thread tasks
    if (!ExecuteTask(FirstPriority, LastPriority))
    {
      // retry with other tasks
      if (!ezTaskSystem::IsTaskGroupFinished(Group) && !ExecuteTask(ezTaskPriority::EarlyThisFrame, ezTaskPriority::LateNextFrame))
      {
        // 'give up'
        ezThreadUtils::YieldTimeSlice();
      }
    }
  }
}

ezResult ezTaskSystem::CancelGroup(ezTaskGroupID Group, ezOnTaskRunning::Enum OnTaskRunning)
{
  if (ezTaskSystem::IsTaskGroupFinished(Group))
    return EZ_SUCCESS;

  EZ_PROFILE(s_ProfileCancelGroup);

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

