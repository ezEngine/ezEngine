#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/SharedPtr.h>

/// \internal Represents the state of a group of tasks that can be waited on
class ezTaskGroup
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTaskGroup);

public:
  ezTaskGroup();
  ~ezTaskGroup();

private:
  friend class ezTaskSystem;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  static void DebugCheckTaskGroup(ezTaskGroupID groupID, ezMutex& mutex);
#else
  EZ_ALWAYS_INLINE static void DebugCheckTaskGroup(ezTaskGroupID groupID, ezMutex& mutex)
  {
    EZ_IGNORE_UNUSED(groupID);
    EZ_IGNORE_UNUSED(mutex);
  }
#endif

  /// \brief Puts the calling thread to sleep until this group is fully finished.
  void WaitForFinish(ezTaskGroupID group) const;
  void Reuse(ezTaskPriority::Enum priority, ezOnTaskGroupFinishedCallback callback);

  bool m_bInUse = true;
  bool m_bStartedByUser = false;
  ezUInt16 m_uiTaskGroupIndex = 0xFFFF; // only there as a debugging aid
  ezUInt32 m_uiGroupCounter = 1;
  ezHybridArray<ezSharedPtr<ezTask>, 16> m_Tasks;
  ezHybridArray<ezTaskGroupID, 4> m_DependsOnGroups;
  ezHybridArray<ezTaskGroupID, 8> m_OthersDependingOnMe;
  ezAtomicInteger32 m_iNumActiveDependencies;
  ezAtomicInteger32 m_iNumRemainingTasks;
  ezOnTaskGroupFinishedCallback m_OnFinishedCallback;
  ezTaskPriority::Enum m_Priority = ezTaskPriority::ThisFrame;
  mutable ezConditionVariable m_CondVarGroupFinished;
};
