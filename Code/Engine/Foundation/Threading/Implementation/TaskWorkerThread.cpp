#include <FoundationPCH.h>

#include <Foundation/Threading/Implementation/TaskWorkerThread.h>

static const char* GenerateThreadName(ezWorkerThreadType::Enum ThreadType, ezUInt32 uiThreadNumber)
{
  static ezStringBuilder sTemp;
  sTemp.Format("{} {}", ezWorkerThreadType::GetThreadTypeName(ThreadType), uiThreadNumber);
  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 uiThreadNumber)
  : ezThread(GenerateThreadName(ThreadType, uiThreadNumber))
{
  m_WorkerType = ThreadType;
  m_uiWorkerThreadNumber = uiThreadNumber & 0xFFFF;
}

ezTaskWorkerThread::~ezTaskWorkerThread() = default;

ezResult ezTaskWorkerThread::DeactivateWorker()
{
  m_bActive = false;

  if (GetThreadStatus() != ezThread::Finished)
  {
    // if necessary, wake this thread up
    WakeUpIfIdle();

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
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
      m_StartedWorkingTime = ezTime::Now();
    }

    if (!ezTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, ezTaskGroupID(), &m_bIsIdle))
    {
      WaitForWork();
    }
    else
    {
      m_iNumTasksExecuted.Increment();
    }
  }

  return 0;
}

void ezTaskWorkerThread::WaitForWork()
{
  // m_bIsIdle usually will be true here, but may also already have been reset to false
  // in that case m_WakeUpSignal will be raised already and the code below will just run through and continue

  m_ThreadActiveTime += ezTime::Now() - m_StartedWorkingTime;
  m_bExecutingTask = false;
  ezTaskSystem::s_IdleWorkerThreads[m_WorkerType].Increment();
  m_WakeUpSignal.WaitForSignal();
  EZ_ASSERT_DEV(m_bIsIdle == false, "Idle state should have been reset");
  ezTaskSystem::s_IdleWorkerThreads[m_WorkerType].Decrement();
}

ezResult ezTaskWorkerThread::WakeUpIfIdle()
{
  if (m_bIsIdle.Set(false) == true) // was idle before
  {
    m_WakeUpSignal.RaiseSignal();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezTaskWorkerThread::UpdateThreadUtilization(ezTime TimePassed)
{
  ezTime tActive = m_ThreadActiveTime;

  // The thread keeps track of how much time it spends executing tasks.
  // Here we retrieve that time and resets it to zero.
  {
    m_ThreadActiveTime = ezTime::Zero();

    if (m_bExecutingTask)
    {
      const ezTime tNow = ezTime::Now();
      tActive += tNow - m_StartedWorkingTime;
      m_StartedWorkingTime = tNow;
    }
  }

  m_fLastThreadUtilization = tActive.GetSeconds() / TimePassed.GetSeconds();
  m_uiLastNumTasksExecuted = m_iNumTasksExecuted.Set(0);
}

double ezTaskWorkerThread::GetThreadUtilization(ezUInt32* pNumTasksExecuted /*= nullptr*/)
{
  if (pNumTasksExecuted)
  {
    *pNumTasksExecuted = m_uiLastNumTasksExecuted;
  }

  return m_fLastThreadUtilization;
}

