#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

const char* ezWorkerThreadType::GetThreadTypeName(ezWorkerThreadType::Enum ThreadType)
{
  switch (ThreadType)
  {
    case ezWorkerThreadType::ShortTasks:
      return "Short Task";

    case ezWorkerThreadType::LongTasks:
      return "Long Task";

    case ezWorkerThreadType::FileAccess:
      return "File Access";
  }

  EZ_REPORT_FAILURE("Invalid Thread Type");
  return "unknown";
}

void ezTaskSystem::StopWorkerThreads()
{
  bool bWorkersStillRunning = true;

  // as long as any worker thread is still active, send the wake up signal
  while (bWorkersStillRunning)
  {
    bWorkersStillRunning = false;

    for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
    {
      const ezUInt32 uiNumThreads = s_iNumWorkerThreads[type];

      for (ezUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_WorkerThreads[type][i]->DeactivateWorker().Failed())
        {
          bWorkersStillRunning = true;
        }
      }
    }

    // waste some time
    ezThreadUtils::YieldTimeSlice();
  }

  for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
  {
    const ezUInt32 uiNumWorkers = s_iNumWorkerThreads[type];

    for (ezUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_WorkerThreads[type][i]->Join();
      EZ_DEFAULT_DELETE(s_WorkerThreads[type][i]);
    }

    s_iNumWorkerThreads[type] = 0;
    s_IdleWorkerThreads[type] = 0;
    s_MaxWorkerThreadsToUse[type] = 0;
    s_WorkerThreads[type].Clear();
  }
}

void ezTaskSystem::SetWorkerThreadCount(ezInt8 iShortTasks, ezInt8 iLongTasks)
{
  ezSystemInformation info = ezSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage
  //
  const ezInt8 iCpuCores = info.GetCPUCoreCount();

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = ezMath::Clamp<ezInt8>(iCpuCores - 2, 2, 6);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = ezMath::Clamp<ezInt8>(iCpuCores - 2, 2, 6);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  iShortTasks = ezMath::Max<ezInt8>(iShortTasks, 1);
  iLongTasks = ezMath::Max<ezInt8>(iLongTasks, 1);

  // if nothing has changed, do nothing
  if (s_MaxWorkerThreadsToUse[ezWorkerThreadType::ShortTasks] == iShortTasks &&
      s_MaxWorkerThreadsToUse[ezWorkerThreadType::LongTasks] == iLongTasks)
    return;

  StopWorkerThreads();

  // this only allocates pointers, ie. the maximum possible number of threads that way may be able to realloc at runtime
  s_WorkerThreads[ezWorkerThreadType::ShortTasks].SetCount(1024);
  s_WorkerThreads[ezWorkerThreadType::LongTasks].SetCount(1024);
  s_WorkerThreads[ezWorkerThreadType::FileAccess].SetCount(128);

  s_MaxWorkerThreadsToUse[ezWorkerThreadType::ShortTasks] = iShortTasks;
  s_MaxWorkerThreadsToUse[ezWorkerThreadType::LongTasks] = iLongTasks;
  s_MaxWorkerThreadsToUse[ezWorkerThreadType::FileAccess] = 1;

  AllocateThreads(ezWorkerThreadType::ShortTasks, s_MaxWorkerThreadsToUse[ezWorkerThreadType::ShortTasks]);
  AllocateThreads(ezWorkerThreadType::LongTasks, s_MaxWorkerThreadsToUse[ezWorkerThreadType::LongTasks]);
  AllocateThreads(ezWorkerThreadType::FileAccess, s_MaxWorkerThreadsToUse[ezWorkerThreadType::FileAccess]);
}

void ezTaskSystem::AllocateThreads(ezWorkerThreadType::Enum type, ezUInt32 uiAddThreads)
{
  EZ_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    EZ_LOCK(s_TaskSystemMutex);

    ezUInt32 uiNextThreadIdx = s_iNumWorkerThreads[type];

    EZ_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_WorkerThreads[type].GetCount(), "Max number of worker threads ({}) exceeded.", s_WorkerThreads[type].GetCount());

    for (ezUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_WorkerThreads[type][uiNextThreadIdx] = EZ_DEFAULT_NEW(ezTaskWorkerThread, (ezWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_WorkerThreads[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_iNumWorkerThreads[type] = uiNextThreadIdx;
  }

  ezLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, ezWorkerThreadType::GetThreadTypeName(type), s_iNumWorkerThreads[type]);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkers);
