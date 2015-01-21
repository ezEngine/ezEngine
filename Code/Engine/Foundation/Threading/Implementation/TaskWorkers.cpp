#include <Foundation/PCH.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Math.h>
#include <Foundation/System/SystemInformation.h>

// Helper function to generate a nice thread name.
static const char* GenerateThreadName(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber)
{
  static ezStringBuilder sTemp;

  switch (ThreadType)
  {
  case ezWorkerThreadType::ShortTasks:
    sTemp.Format("Short Tasks %i", iThreadNumber + 1);
    break;
  case ezWorkerThreadType::LongTasks:
    sTemp.Format("Long Tasks %i", iThreadNumber + 1);
    break;
  case ezWorkerThreadType::FileAccess:
    if (iThreadNumber > 0)
      sTemp.Format("Resource Loading %i", iThreadNumber + 1);
    else
      sTemp = "Resource Loading";
    break;
  case ezWorkerThreadType::ENUM_COUNT:
    EZ_REPORT_FAILURE("Invalid Thread Type");
    break;
  }

  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber) : ezThread(GenerateThreadName(ThreadType, iThreadNumber))
{
  m_bActive = true;
  m_WorkerType = ThreadType;
  m_uiWorkerThreadNumber = iThreadNumber;

  m_bExecutingTask = false;
  m_ThreadUtilization = 0.0;
  m_iTasksExecutionCounter = 0;
  m_uiNumTasksExecuted = 0;
}

bool ezTaskSystem::IsLoadingThread()
{
  if (s_WorkerThreads[ezWorkerThreadType::FileAccess].IsEmpty())
    return false;

  return ezThreadUtils::GetCurrentThreadID() == s_WorkerThreads[ezWorkerThreadType::FileAccess][0]->GetThreadID();
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

void ezTaskSystem::SetWorkThreadCount(ezInt8 iShortTasks, ezInt8 iLongTasks)
{
  ezSystemInformation info = ezSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage

  // 1 on single core, dual core, tri core CPUs, 2 on Quad core, 4 on six cores and up
  if (iShortTasks <= 0)
    iShortTasks = ezMath::Clamp<ezInt8>(info.GetCPUCoreCount() - 2, 1, 4);

  // 1 on single core, dual core, tri core CPUs, 2 on Quad core, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = ezMath::Clamp<ezInt8>(info.GetCPUCoreCount() - 2, 1, 6);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  iShortTasks = ezMath::Max<ezInt8>(iShortTasks, 1);
  iLongTasks  = ezMath::Max<ezInt8>(iLongTasks, 1);

  // if nothing has changed, do nothing
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == iShortTasks && s_WorkerThreads[ezWorkerThreadType::LongTasks].GetCount() == iLongTasks)
    return;

  StopWorkerThreads();

  s_WorkerThreads[ezWorkerThreadType::ShortTasks].SetCount(iShortTasks);
  s_WorkerThreads[ezWorkerThreadType::LongTasks].SetCount(iLongTasks);
  s_WorkerThreads[ezWorkerThreadType::FileAccess].SetCount(1);

  for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
  {
    for (ezUInt32 i = 0; i < s_WorkerThreads[type].GetCount(); ++i)
    {
      s_WorkerThreads[type][i] = EZ_DEFAULT_NEW(ezTaskWorkerThread)((ezWorkerThreadType::Enum) type, i);
      s_WorkerThreads[type][i]->Start();
    }
  }
}

ezUInt32 ezTaskWorkerThread::Run()
{
  ezTaskPriority::Enum FirstPriority = ezTaskPriority::EarlyThisFrame;
  ezTaskPriority::Enum LastPriority = ezTaskPriority::LateNextFrame;

  if (m_WorkerType == ezWorkerThreadType::LongTasks)
  {
    FirstPriority = ezTaskPriority::LongRunningHighPriority;
    LastPriority = ezTaskPriority::LongRunning;
  }
  else
  if (m_WorkerType == ezWorkerThreadType::FileAccess)
  {
    FirstPriority = ezTaskPriority::FileAccessHighPriority;
    LastPriority = ezTaskPriority::FileAccess;
  }

  EZ_ASSERT_DEBUG(m_WorkerType < ezWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: %i", m_WorkerType);

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

