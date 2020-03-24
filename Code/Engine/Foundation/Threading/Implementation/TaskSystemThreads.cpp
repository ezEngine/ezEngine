#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezUInt32 ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::Enum type)
{
  return s_ThreadState->m_uiMaxWorkersToUse[type];
}

ezUInt32 ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::Enum type)
{
  return s_ThreadState->m_iAllocatedWorkers[type];
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
    iShortTasks = ezMath::Clamp<ezInt8>(iCpuCores - 2, 2, 8);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = ezMath::Clamp<ezInt8>(iCpuCores - 2, 2, 8);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  iShortTasks = ezMath::Max<ezInt8>(iShortTasks, 1);
  iLongTasks = ezMath::Max<ezInt8>(iLongTasks, 1);

  // if nothing has changed, do nothing
  if (s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks] == iShortTasks &&
      s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks] == iLongTasks)
    return;

  StopWorkerThreads();

  // this only allocates pointers, ie. the maximum possible number of threads that we may be able to realloc at runtime
  s_ThreadState->m_Workers[ezWorkerThreadType::ShortTasks].SetCount(1024);
  s_ThreadState->m_Workers[ezWorkerThreadType::LongTasks].SetCount(1024);
  s_ThreadState->m_Workers[ezWorkerThreadType::FileAccess].SetCount(128);

  s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks] = iShortTasks;
  s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks] = iLongTasks;
  s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::FileAccess] = 1;

  AllocateThreads(ezWorkerThreadType::ShortTasks, s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks]);
  AllocateThreads(ezWorkerThreadType::LongTasks, s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks]);
  AllocateThreads(ezWorkerThreadType::FileAccess, s_ThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::FileAccess]);
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
      const ezUInt32 uiNumThreads = s_ThreadState->m_iAllocatedWorkers[type];

      for (ezUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_ThreadState->m_Workers[type][i]->DeactivateWorker().Failed())
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
    const ezUInt32 uiNumWorkers = s_ThreadState->m_iAllocatedWorkers[type];

    for (ezUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_ThreadState->m_Workers[type][i]->Join();
      EZ_DEFAULT_DELETE(s_ThreadState->m_Workers[type][i]);
    }

    s_ThreadState->m_iAllocatedWorkers[type] = 0;
    s_ThreadState->m_iNumIdleWorkers[type] = 0;
    s_ThreadState->m_uiMaxWorkersToUse[type] = 0;
    s_ThreadState->m_Workers[type].Clear();
  }
}

void ezTaskSystem::AllocateThreads(ezWorkerThreadType::Enum type, ezUInt32 uiAddThreads)
{
  EZ_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    EZ_LOCK(s_TaskSystemMutex);

    ezUInt32 uiNextThreadIdx = s_ThreadState->m_iAllocatedWorkers[type];

    EZ_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_ThreadState->m_Workers[type].GetCount(), "Max number of worker threads ({}) exceeded.", s_ThreadState->m_Workers[type].GetCount());

    for (ezUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_ThreadState->m_Workers[type][uiNextThreadIdx] = EZ_DEFAULT_NEW(ezTaskWorkerThread, (ezWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_ThreadState->m_Workers[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_ThreadState->m_iAllocatedWorkers[type] = uiNextThreadIdx;
  }

  ezLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, ezWorkerThreadType::GetThreadTypeName(type), s_ThreadState->m_iAllocatedWorkers[type]);
}

ezInt32 ezTaskSystem::CalcActivatableThreads(ezWorkerThreadType::Enum type)
{
  const ezUInt32 uiTotal = s_ThreadState->m_iAllocatedWorkers[type];
  const ezUInt32 uiBlocked = s_ThreadState->m_iNumBlockedWorkers[type];
  const ezUInt32 uiIdle = s_ThreadState->m_iNumIdleWorkers[type];
  const ezUInt32 uiMaxActive = s_ThreadState->m_uiMaxWorkersToUse[type];

  EZ_ASSERT_DEV(uiBlocked <= uiTotal, "Incorrect worker thread statistics");
  EZ_ASSERT_DEV(uiIdle <= uiTotal, "Incorrect worker thread statistics");
  EZ_ASSERT_DEV(uiIdle + uiBlocked <= uiTotal, "Incorrect worker thread statistics");

  const ezUInt32 uiActive = uiTotal - uiBlocked - uiIdle;

  const ezInt32 iCanActivate = (ezInt32)uiMaxActive - (ezInt32)uiActive;

  return iCanActivate;
}

ezResult ezTaskSystem::WakeUpThreadIfIdle(ezWorkerThreadType::Enum type, ezUInt32 threadIdx)
{
  if (s_ThreadState->m_Workers[type][threadIdx]->WakeUpIfIdle().Succeeded())
  {
    // the thread index must be different, if it is the same, it must be an entirely different worker thread type
    EZ_ASSERT_DEV(threadIdx != tl_TaskWorkerInfo.m_iWorkerIndex || type != tl_TaskWorkerInfo.m_WorkerType, "Calling thread was in idle state itself.");

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezTaskSystem::WakeUpThreads(ezWorkerThreadType::Enum type, ezUInt32 uiNumThreadsToWakeUp)
{
  const ezInt32 iCanActivate = CalcActivatableThreads(type);
  ezInt32 iShouldActivate = ezMath::Min<ezInt32>(iCanActivate, uiNumThreadsToWakeUp);

  const ezUInt32 uiTotal = s_ThreadState->m_iAllocatedWorkers[type];

  for (ezUInt32 threadIdx = 0; threadIdx < uiTotal && iShouldActivate > 0; ++threadIdx)
  {
    if (WakeUpThreadIfIdle(type, threadIdx).Succeeded())
    {
      --iShouldActivate;
    }
  }

  if (iShouldActivate > 0)
  {
    // the new threads will start not-idle and take on some work
    AllocateThreads(type, iShouldActivate);
  }
}

ezWorkerThreadType::Enum ezTaskSystem::GetCurrentThreadWorkerType()
{
  return tl_TaskWorkerInfo.m_WorkerType;
}

double ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::Enum Type, ezUInt32 uiThreadIndex, ezUInt32* pNumTasksExecuted /*= nullptr*/)
{
  return s_ThreadState->m_Workers[Type][uiThreadIndex]->GetThreadUtilization(pNumTasksExecuted);
}

void ezTaskSystem::DetermineTasksToExecuteOnThread(ezTaskPriority::Enum& out_FirstPriority, ezTaskPriority::Enum& out_LastPriority)
{
  switch (tl_TaskWorkerInfo.m_WorkerType)
  {
    case ezWorkerThreadType::MainThread:
    {
      out_FirstPriority = ezTaskPriority::ThisFrameMainThread;
      out_LastPriority = ezTaskPriority::SomeFrameMainThread;
      break;
    }

    case ezWorkerThreadType::FileAccess:
    {
      out_FirstPriority = ezTaskPriority::FileAccessHighPriority;
      out_LastPriority = ezTaskPriority::FileAccess;
      break;
    }

    case ezWorkerThreadType::LongTasks:
    {
      out_FirstPriority = ezTaskPriority::LongRunningHighPriority;
      out_LastPriority = ezTaskPriority::LongRunning;
      break;
    }

    case ezWorkerThreadType::ShortTasks:
    {
      out_FirstPriority = ezTaskPriority::EarlyThisFrame;
      out_LastPriority = ezTaskPriority::In9Frames;
      break;
    }

    case ezWorkerThreadType::Unknown:
    {
      // probably a thread not launched through ez
      out_FirstPriority = ezTaskPriority::EarlyThisFrame;
      out_LastPriority = ezTaskPriority::In9Frames;
      break;
    }

    default:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemThreads);
