#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>

ezEvent<const ezThreadEvent&, ezNoMutex> ezThread::s_ThreadEvents;

ezThread::ezThread(const char* szName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
  : ezOSThread(ezThreadClassEntryPoint, this, szName, uiStackSize)
  , m_ThreadStatus(Created)
  , m_Name(szName)
{
  ezThreadEvent e;
  e.m_pThread = this;
  e.m_Type = ezThreadEvent::Type::ThreadCreated;
  ezThread::s_ThreadEvents.Broadcast(e, 255);
}

 ezThread::~ezThread()
{
  EZ_ASSERT_DEV(!IsRunning(), "Thread deletion while still running detected!");

  ezThreadEvent e;
  e.m_pThread = this;
  e.m_Type = ezThreadEvent::Type::ThreadDestroyed;
  ezThread::s_ThreadEvents.Broadcast(e, 255);
}

// Deactivate Doxygen document generation for the following block.
/// \cond
 
ezUInt32 RunThread(ezThread* pThread)
{
  if (pThread == nullptr)
    return 0;

  ezProfilingSystem::SetThreadName(pThread->m_Name.GetData());

  {
    ezThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = ezThreadEvent::Type::StartingExecution;
    ezThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = ezThread::Running;

  // Run the worker thread function
  ezUInt32 uiReturnCode = pThread->Run();

  {
    ezThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = ezThreadEvent::Type::FinishedExecution;
    ezThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = ezThread::Finished;

  ezProfilingSystem::RemoveThread();

  return uiReturnCode;
}

/// \endcond

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Thread_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/Threading/Implementation/Posix/Thread_posix.h>
#else
#  error "Runnable thread entry functions are not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Thread);
