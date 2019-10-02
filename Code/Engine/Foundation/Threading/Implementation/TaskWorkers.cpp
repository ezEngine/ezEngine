#include <FoundationPCH.h>

#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/TaskSystem.h>

extern thread_local ezWorkerThreadType::Enum g_ThreadTaskType;

// Helper function to generate a nice thread name.
static const char* GenerateThreadName(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber)
{
  static ezStringBuilder sTemp;

  switch (ThreadType)
  {
    case ezWorkerThreadType::ShortTasks:
      sTemp.Format("Short Tasks {0}", iThreadNumber + 1);
      break;
    case ezWorkerThreadType::LongTasks:
      sTemp.Format("Long Tasks {0}", iThreadNumber + 1);
      break;
    case ezWorkerThreadType::FileAccess:
      if (iThreadNumber > 0)
        sTemp.Format("File Access {0}", iThreadNumber + 1);
      else
        sTemp = "File Access";
      break;

    default:
      EZ_REPORT_FAILURE("Invalid Thread Type");
      break;
  }

  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber)
  : ezThread(GenerateThreadName(ThreadType, iThreadNumber))
{
  m_bActive = true;
  m_WorkerType = ThreadType;
  m_uiWorkerThreadNumber = iThreadNumber;

  m_bExecutingTask = false;
  m_ThreadUtilization = 0.0;
  m_iTasksExecutionCounter = 0;
  m_uiNumTasksExecuted = 0;
}

void ezTaskSystem::StopWorkerThreads()
{
  // tell all threads that they should terminate
  for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
  {
    for (ezUInt32 i = 0; i < s_WorkerThreads[type].GetCount(); ++i)
    {
      s_WorkerThreads[type][i]->m_bActive = false;
    }
  }

  bool bWorkersStillRunning = true;

  // as long as any worker thread is still active, send the wake up signal
  while (bWorkersStillRunning)
  {
    bWorkersStillRunning = false;

    for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
    {
      for (ezUInt32 i = 0; i < s_WorkerThreads[type].GetCount(); ++i)
      {
        if (s_WorkerThreads[type][i]->GetThreadStatus() != ezThread::Finished)
        {
          bWorkersStillRunning = true;

          // send a signal
          s_TasksAvailableSignal[type].RaiseSignal();

          // waste some time
          ezThreadUtils::YieldTimeSlice();
        }
      }
    }
  }

  for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
  {
    for (ezUInt32 i = 0; i < s_WorkerThreads[type].GetCount(); ++i)
    {
      s_WorkerThreads[type][i]->Join();
      EZ_DEFAULT_DELETE(s_WorkerThreads[type][i]);
    }

    s_WorkerThreads[type].Clear();
  }
}

void ezTaskSystem::SetWorkerThreadCount(ezInt8 iShortTasks, ezInt8 iLongTasks)
{
  ezSystemInformation info = ezSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = ezMath::Clamp<ezInt8>(info.GetCPUCoreCount() - 2, 2, 6);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = ezMath::Clamp<ezInt8>(info.GetCPUCoreCount() - 2, 2, 6);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  iShortTasks = ezMath::Max<ezInt8>(iShortTasks, 1);
  iLongTasks = ezMath::Max<ezInt8>(iLongTasks, 1);

  // if nothing has changed, do nothing
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == iShortTasks &&
      s_WorkerThreads[ezWorkerThreadType::LongTasks].GetCount() == iLongTasks)
    return;

  StopWorkerThreads();

  s_WorkerThreads[ezWorkerThreadType::ShortTasks].SetCount(iShortTasks);
  s_WorkerThreads[ezWorkerThreadType::LongTasks].SetCount(iLongTasks);
  s_WorkerThreads[ezWorkerThreadType::FileAccess].SetCount(1);

  for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
  {
    for (ezUInt32 i = 0; i < s_WorkerThreads[type].GetCount(); ++i)
    {
      s_WorkerThreads[type][i] = EZ_DEFAULT_NEW(ezTaskWorkerThread, (ezWorkerThreadType::Enum)type, i);
      s_WorkerThreads[type][i]->Start();
    }
  }
}

ezUInt32 ezTaskWorkerThread::Run()
{
  EZ_ASSERT_DEBUG(m_WorkerType != ezWorkerThreadType::Unknown && m_WorkerType != ezWorkerThreadType::MainThread, "Worker threads cannot use this type");
  EZ_ASSERT_DEBUG(m_WorkerType < ezWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // once this thread is running, store the worker type in the thread_local variable
  // such that the ezTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  g_ThreadTaskType = m_WorkerType;

  bool bAllowDefaultWork;
  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::In9Frames;
  ezTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority, bAllowDefaultWork);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (!m_bExecutingTask)
    {
      m_bExecutingTask = true;
      m_StartedWorking = ezTime::Now();
    }

    if (!ezTaskSystem::ExecuteTask(FirstPriority, LastPriority))
    {
      // if no work is currently available, wait for the signal that new work has been added

      m_ThreadActiveTime += ezTime::Now() - m_StartedWorking;
      m_bExecutingTask = false;
      ezTaskSystem::s_TasksAvailableSignal[m_WorkerType].WaitForSignal();
    }
    else
      m_iTasksExecutionCounter.Increment();
  }

  return 0;
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
  m_ThreadActiveTime = ezTime::Seconds(0.0);

  if (m_bExecutingTask)
  {
    const ezTime tNow = ezTime::Now();
    tActive += tNow - m_StartedWorking;
    m_StartedWorking = tNow;
  }

  return tActive;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkers);
