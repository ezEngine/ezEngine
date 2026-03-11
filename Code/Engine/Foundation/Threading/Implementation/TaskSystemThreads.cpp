#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

ezUInt32 ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::Enum type)
{
  return s_pThreadState->m_uiMaxWorkersToUse[type];
}

ezUInt32 ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::Enum type)
{
  return s_pThreadState->m_iAllocatedWorkers[type];
}

void ezTaskSystem::SetWorkerThreadCount(ezInt32 iShortTasks, ezInt32 iLongTasks)
{
  ezSystemInformation info = ezSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage
  //
  const ezInt32 iCpuCores = info.GetCPUCoreCount();

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = ezMath::Clamp<ezInt32>(iCpuCores - 2, 2, 8);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = ezMath::Clamp<ezInt32>(iCpuCores - 2, 2, 8);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  ezUInt32 uiShortTasks = static_cast<ezUInt32>(ezMath::Max<ezInt32>(iShortTasks, 1));
  ezUInt32 uiLongTasks = static_cast<ezUInt32>(ezMath::Max<ezInt32>(iLongTasks, 1));

  // if nothing has changed, do nothing
  if (s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks] == uiShortTasks &&
      s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks] == uiLongTasks)
    return;

  ezLog::Dev("CPU core count: {}", iCpuCores);
  ezLog::Dev("Setting worker thread count to {} (short) / {} (long).", uiShortTasks, uiLongTasks);

  StopWorkerThreads();

  // this only allocates pointers, i.e. the maximum possible number of threads that we may be able to realloc at runtime
  s_pThreadState->m_Workers[ezWorkerThreadType::ShortTasks].SetCount(1024);
  s_pThreadState->m_Workers[ezWorkerThreadType::LongTasks].SetCount(1024);
  s_pThreadState->m_Workers[ezWorkerThreadType::FileAccess].SetCount(128);

  s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks] = uiShortTasks;
  s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks] = uiLongTasks;
  s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::FileAccess] = 1;

  AllocateThreads(ezWorkerThreadType::ShortTasks, s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::ShortTasks]);
  AllocateThreads(ezWorkerThreadType::LongTasks, s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::LongTasks]);
  AllocateThreads(ezWorkerThreadType::FileAccess, s_pThreadState->m_uiMaxWorkersToUse[ezWorkerThreadType::FileAccess]);
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
      const ezUInt32 uiNumThreads = s_pThreadState->m_iAllocatedWorkers[type];

      for (ezUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_pThreadState->m_Workers[type][i]->DeactivateWorker().Failed())
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
    const ezUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

    for (ezUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_pThreadState->m_Workers[type][i]->Join();
      EZ_DEFAULT_DELETE(s_pThreadState->m_Workers[type][i]);
    }

    s_pThreadState->m_iAllocatedWorkers[type] = 0;
    s_pThreadState->m_uiMaxWorkersToUse[type] = 0;
    s_pThreadState->m_Workers[type].Clear();
  }
}

void ezTaskSystem::AllocateThreads(ezWorkerThreadType::Enum type, ezUInt32 uiAddThreads)
{
  EZ_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    EZ_LOCK(s_TaskSystemMutex);

    ezUInt32 uiNextThreadIdx = s_pThreadState->m_iAllocatedWorkers[type];

    EZ_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_pThreadState->m_Workers[type].GetCount(), "Max number of worker threads ({}) exceeded.",
      s_pThreadState->m_Workers[type].GetCount());

    for (ezUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_pThreadState->m_Workers[type][uiNextThreadIdx] = EZ_DEFAULT_NEW(ezTaskWorkerThread, (ezWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_pThreadState->m_Workers[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_pThreadState->m_iAllocatedWorkers[type] = uiNextThreadIdx;
  }

  ezLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, ezWorkerThreadType::GetThreadTypeName(type),
    s_pThreadState->m_iAllocatedWorkers[type]);
}

void ezTaskSystem::WakeUpThreads(ezWorkerThreadType::Enum type, ezUInt32 uiNumThreadsToWakeUp)
{
  // together with ezTaskWorkerThread::Run() this function will make sure to keep the number
  // of active threads close to m_uiMaxWorkersToUse
  //
  // threads that go into the 'blocked' state will raise the number of threads that get activated
  // and when they are unblocked, together they may exceed the 'maximum' number of active threads
  // but over time the threads at the end of the list will put themselves to sleep again

  auto* s = ezTaskSystem::s_pThreadState.Borrow();

  const ezUInt32 uiTotalThreads = s_pThreadState->m_iAllocatedWorkers[type];
  ezUInt32 uiAllowedActiveThreads = s_pThreadState->m_uiMaxWorkersToUse[type];

  for (ezUInt32 threadIdx = 0; threadIdx < uiTotalThreads; ++threadIdx)
  {
    switch (s->m_Workers[type][threadIdx]->WakeUpIfIdle())
    {
      case ezTaskWorkerState::Idle:
      {
        // was idle before -> now it is active
        if (--uiNumThreadsToWakeUp == 0)
          return;

        [[fallthrough]];
      }

      case ezTaskWorkerState::Active:
      {
        // already active
        if (--uiAllowedActiveThreads == 0)
          return;

        break;
      }

      default:
        break;
    }
  }

  // if the loop above did not find enough threads to wake up
  if (uiNumThreadsToWakeUp > 0 && uiAllowedActiveThreads > 0)
  {
    // the new threads will start not-idle and take on some work
    AllocateThreads(type, ezMath::Min(uiNumThreadsToWakeUp, uiAllowedActiveThreads));
  }
}

ezWorkerThreadType::Enum ezTaskSystem::GetCurrentThreadWorkerType()
{
  return tl_TaskWorkerInfo.m_WorkerType;
}

double ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::Enum type, ezUInt32 uiThreadIndex, ezUInt32* pNumTasksExecuted /*= nullptr*/)
{
  return s_pThreadState->m_Workers[type][uiThreadIndex]->GetThreadUtilization(pNumTasksExecuted);
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
