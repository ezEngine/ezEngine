#pragma once

#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

class ezTaskSystemThreadState
{
private:
  friend class ezTaskSystem;
  friend class ezTaskWorkerThread;

  // only for debugging
  ezAtomicInteger32 s_IdleWorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // need to know how many threads are non-idle but blocked
  ezAtomicInteger32 s_BlockedWorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // The arrays of all the active worker threads.
  ezDynamicArray<ezTaskWorkerThread*> s_WorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // the number of allocated (non-null) worker threads in s_WorkerThreads
  ezAtomicInteger32 s_iNumWorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  ezUInt32 s_MaxWorkerThreadsToUse[ezWorkerThreadType::ENUM_COUNT] = {};
};
