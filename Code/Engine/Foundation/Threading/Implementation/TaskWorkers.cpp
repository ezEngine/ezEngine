#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/TaskSystem.h>

extern thread_local ezWorkerThreadType::Enum g_ThreadTaskType;
extern thread_local ezInt32 g_iWorkerThreadIdx;

static const char* GetThreadTypeName(ezWorkerThreadType::Enum ThreadType)
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

// Helper function to generate a nice thread name.
static const char* GenerateThreadName(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber)
{
  static ezStringBuilder sTemp;
  sTemp.Format("{} {}", GetThreadTypeName(ThreadType), iThreadNumber);
  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber)
  : ezThread(GenerateThreadName(ThreadType, iThreadNumber))
{
  m_WorkerType = ThreadType;
  m_uiWorkerThreadNumber = iThreadNumber;
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
      const ezUInt32 uiNumWorkers = s_iNumWorkerThreads[type];

      for (ezUInt32 i = 0; i < uiNumWorkers; ++i)
      {
        s_WorkerThreads[type][i]->m_bActive = false;

        if (s_WorkerThreads[type][i]->GetThreadStatus() != ezThread::Finished)
        {
          bWorkersStillRunning = true;

          // wake this thread up if necessary
          WakeUpIdleThread(static_cast<ezWorkerThreadType::Enum>(type), i);

          // waste some time
          ezThreadUtils::YieldTimeSlice();
        }
      }
    }
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

  ezLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, GetThreadTypeName(type), s_iNumWorkerThreads[type]);
}

ezUInt32 ezTaskWorkerThread::Run()
{
  EZ_ASSERT_DEBUG(m_WorkerType != ezWorkerThreadType::Unknown && m_WorkerType != ezWorkerThreadType::MainThread, "Worker threads cannot use this type");
  EZ_ASSERT_DEBUG(m_WorkerType < ezWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // once this thread is running, store the worker type in the thread_local variable
  // such that the ezTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  g_ThreadTaskType = m_WorkerType;
  g_iWorkerThreadIdx = m_uiWorkerThreadNumber;

  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::In9Frames;
  ezTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (!m_bExecutingTask)
    {
      m_bExecutingTask = true;
      m_StartedWorking = ezTime::Now();
    }

    if (!ezTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, ezTaskGroupID(), &m_bIsIdle))
    {
      Idle();
    }
    else
    {
      m_iTasksExecutionCounter.Increment();
    }
  }

  return 0;
}

void ezTaskWorkerThread::Idle()
{
  // m_bIsIdle usually will be true here, but may also already have been reset to false
  // then the code below will just run through and continue

  m_ThreadActiveTime += ezTime::Now() - m_StartedWorking;
  m_bExecutingTask = false;
  ezTaskSystem::s_IdleWorkerThreads[m_WorkerType].Increment();
  m_WakeUpSignal.WaitForSignal();
  EZ_ASSERT_DEV(m_bIsIdle == false, "Idle state should have been reset");
  ezTaskSystem::s_IdleWorkerThreads[m_WorkerType].Decrement();
}

void ezTaskWorkerThread::ComputeThreadUtilization(ezTime TimePassed)
{
  const ezTime tActive = GetAndResetThreadActiveTime();

  m_ThreadUtilization = tActive.GetSeconds() / TimePassed.GetSeconds();
  m_uiNumTasksExecuted = m_iTasksExecutionCounter.Set(0);
}

ezTime ezTaskWorkerThread::GetAndResetThreadActiveTime()
{
  ezTime tActive = m_ThreadActiveTime;
  m_ThreadActiveTime = ezTime::Zero();

  if (m_bExecutingTask)
  {
    const ezTime tNow = ezTime::Now();
    tActive += tNow - m_StartedWorking;
    m_StartedWorking = tNow;
  }

  return tActive;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkers);
