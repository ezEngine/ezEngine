#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

thread_local ezTaskWorkerInfo tl_TaskWorkerInfo;

static const char* GenerateThreadName(ezWorkerThreadType::Enum ThreadType, ezUInt32 uiThreadNumber)
{
  static ezStringBuilder sTemp;
  sTemp.Format("{} {}", ezWorkerThreadType::GetThreadTypeName(ThreadType), uiThreadNumber);
  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 uiThreadNumber)
  : ezThread(GenerateThreadName(ThreadType, uiThreadNumber), 32 * 1024) /* 32 KB of stack size */
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
  EZ_ASSERT_DEBUG(
    m_WorkerType != ezWorkerThreadType::Unknown && m_WorkerType != ezWorkerThreadType::MainThread, "Worker threads cannot use this type");
  EZ_ASSERT_DEBUG(m_WorkerType < ezWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // once this thread is running, store the worker type in the thread_local variable
  // such that the ezTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  tl_TaskWorkerInfo.m_WorkerType = m_WorkerType;
  tl_TaskWorkerInfo.m_iWorkerIndex = m_uiWorkerThreadNumber;
  tl_TaskWorkerInfo.m_pWorkerState = &m_WorkerState;

  const bool bIsReserve = m_uiWorkerThreadNumber >= ezTaskSystem::s_ThreadState->m_uiMaxWorkersToUse[m_WorkerType];

  ezTaskPriority::Enum FirstPriority;
  ezTaskPriority::Enum LastPriority;
  ezTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (!m_bExecutingTask)
    {
      m_bExecutingTask = true;
      m_StartedWorkingTime = ezTime::Now();
    }

    if (!ezTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, ezTaskGroupID(), &m_WorkerState))
    {
      WaitForWork();
    }
    else
    {
      ++m_uiNumTasksExecuted;

      if (bIsReserve)
      {
        EZ_VERIFY(m_WorkerState.Set((int)ezTaskWorkerState::Idle) == (int)ezTaskWorkerState::Active, "Corrupt worker state");

        // if this thread is part of the reserve, then don't continue to process tasks indefinitely
        // instead, put this thread to sleep and wake up someone else
        // that someone else may be a thread at the front of the queue, it may also turn out to be this thread again
        // either way, if at some point we woke up more threads than the maximum desired, this will move the active threads
        // to the front of the list, because of the way ezTaskSystem::WakeUpThreads() works
        ezTaskSystem::WakeUpThreads(m_WorkerType, 1);

        WaitForWork();
      }
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
  m_WakeUpSignal.WaitForSignal();
  EZ_ASSERT_DEBUG(m_WorkerState == (int)ezTaskWorkerState::Active, "Worker state should have been reset to 'active'");
}

ezTaskWorkerState ezTaskWorkerThread::WakeUpIfIdle()
{
  ezTaskWorkerState prev = (ezTaskWorkerState)m_WorkerState.CompareAndSwap((int)ezTaskWorkerState::Idle, (int)ezTaskWorkerState::Active);
  if (prev == ezTaskWorkerState::Idle) // was idle before
  {
    m_WakeUpSignal.RaiseSignal();
  }

  return static_cast<ezTaskWorkerState>(prev);
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
  m_uiLastNumTasksExecuted = m_uiNumTasksExecuted;
  m_uiNumTasksExecuted = 0;
}

double ezTaskWorkerThread::GetThreadUtilization(ezUInt32* pNumTasksExecuted /*= nullptr*/)
{
  if (pNumTasksExecuted)
  {
    *pNumTasksExecuted = m_uiLastNumTasksExecuted;
  }

  return m_fLastThreadUtilization;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkerThread);
