#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Lock.h>

ezTaskGroup::ezTaskGroup() = default;
ezTaskGroup::~ezTaskGroup() = default;

void ezTaskGroup::WaitForFinish(ezTaskGroupID group) const
{
  if (m_uiGroupCounter != group.m_uiGroupCounter)
    return;

  EZ_PROFILE_SCOPE("ezTaskGroup::WaitForFinish");
  EZ_LOCK(m_CondVarGroupFinished);

  while (m_uiGroupCounter == group.m_uiGroupCounter)
  {
    m_CondVarGroupFinished.UnlockWaitForSignalAndLock();
  }
}

void ezTaskGroup::Reuse(ezTaskPriority::Enum priority, ezOnTaskGroupFinishedCallback callback)
{
  m_bInUse = true;
  m_bStartedByUser = false;
  m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  m_Tasks.Clear();
  m_DependsOnGroups.Clear();
  m_OthersDependingOnMe.Clear();
  m_Priority = priority;
  m_OnFinishedCallback = callback;
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
void ezTaskGroup::DebugCheckTaskGroup(ezTaskGroupID groupID, ezMutex& mutex)
{
  EZ_LOCK(mutex);

  const ezTaskGroup* pGroup = groupID.m_pTaskGroup;
  EZ_IGNORE_UNUSED(pGroup);

  EZ_ASSERT_DEV(pGroup != nullptr, "TaskGroupID is invalid.");
  EZ_ASSERT_DEV(pGroup->m_uiGroupCounter == groupID.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  EZ_ASSERT_DEV(!pGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  EZ_ASSERT_DEV(pGroup->m_iNumActiveDependencies == 0, "Invalid active dependenices");
}
#endif
