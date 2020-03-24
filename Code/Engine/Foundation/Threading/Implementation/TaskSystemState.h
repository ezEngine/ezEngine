#pragma once

#include <Foundation/Threading/TaskSystem.h>

class ezTaskSystemThreadState
{
private:
  friend class ezTaskSystem;
  friend class ezTaskWorkerThread;

  // only for debugging
  ezAtomicInteger32 m_iNumIdleWorkers[ezWorkerThreadType::ENUM_COUNT];

  // need to know how many threads are non-idle but blocked
  ezAtomicInteger32 m_iNumBlockedWorkers[ezWorkerThreadType::ENUM_COUNT];

  // The arrays of all the active worker threads.
  ezDynamicArray<ezTaskWorkerThread*> m_Workers[ezWorkerThreadType::ENUM_COUNT];

  // the number of allocated (non-null) worker threads in m_Workers
  ezAtomicInteger32 m_iAllocatedWorkers[ezWorkerThreadType::ENUM_COUNT];

  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  ezUInt32 m_uiMaxWorkersToUse[ezWorkerThreadType::ENUM_COUNT] = {};
};

class ezTaskSystemState
{
private:
  friend class ezTaskSystem;

  // The target frame time used by FinishFrameTasks()
  ezTime m_TargetFrameTime = ezTime::Seconds(1.0 / 40.0); // => 25 ms

  // The deque can grow without relocating existing data, therefore the ezTaskGroupID's can store pointers directly to the data
  ezDeque<ezTaskGroup> m_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  ezList<ezTaskSystem::TaskData> m_Tasks[ezTaskPriority::ENUM_COUNT];
};
