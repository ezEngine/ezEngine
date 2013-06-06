#include <Foundation/PCH.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Math.h>

// Helper function to generate a nice thread name.
static const char* GenerateThreadName(ezWorkerThreadType::Enum ToDo, ezUInt32 iThreadNumber)
{
  static ezStringBuilder sTemp;

  switch (ToDo)
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
  }

  return sTemp.GetData();
}

ezTaskWorkerThread::ezTaskWorkerThread(ezWorkerThreadType::Enum ToDo, ezUInt32 iThreadNumber) : ezThread(GenerateThreadName(ToDo, iThreadNumber))
{
  m_bActive = true;
  m_WorkerType = ToDo;
  m_uiWorkerThreadNumber = iThreadNumber;
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

void ezTaskSystem::SetWorkThreadCount(ezUInt8 uiShortTasks, ezUInt8 uiLongTasks)
{
  EZ_ASSERT(uiShortTasks > 0, "At least one worker thread for short tasks must be active.");
  EZ_ASSERT(uiLongTasks > 0, "At least one worker thread for long tasks must be active.");

  uiShortTasks = ezMath::Max<ezUInt8>(uiShortTasks, 1);
  uiLongTasks  = ezMath::Max<ezUInt8>(uiLongTasks, 1);

  // if nothing has changed, do nothing
  if (s_WorkerThreads[ezWorkerThreadType::ShortTasks].GetCount() == uiShortTasks && s_WorkerThreads[ezWorkerThreadType::LongTasks].GetCount() == uiLongTasks)
    return;

  StopWorkerThreads();

  s_WorkerThreads[ezWorkerThreadType::ShortTasks].SetCount(uiShortTasks);
  s_WorkerThreads[ezWorkerThreadType::LongTasks].SetCount(uiLongTasks);
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

  EZ_ASSERT(m_WorkerType < ezWorkerThreadType::ENUM_COUNT, "Worker Thread Typs is invalid: %i", m_WorkerType);

  while (m_bActive)
  {
    if (!ezTaskSystem::ExecuteTask(FirstPriority, LastPriority))
    {
      // if no work is currently available, wait for the signal that new work has been added

      ezTaskSystem::s_TasksAvailableSignal[m_WorkerType].WaitForSignal();
    }
  }

  return 0;
}
