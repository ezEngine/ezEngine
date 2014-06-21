
#include <Foundation/PCH.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

ezUInt32 RunThread(ezThread* pThread)
{
  if (pThread == nullptr)
    return 0;

  ezThreadLocalStorage::SetPerThreadPointerTable(&(pThread->m_ThreadLocalPointerTable));
  ezProfilingSystem::SetThreadName(pThread->m_Name.GetData());
  
  pThread->m_ThreadStatus = ezThread::Running;

  // Run the worker thread function
  ezUInt32 uiReturnCode = pThread->Run();

  pThread->m_ThreadStatus = ezThread::Finished;

  ezThreadLocalStorage::SetPerThreadPointerTable(nullptr);

  return uiReturnCode;
}

/// \endcond

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/Thread_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/Thread_posix.h>
#else
  #error "Runnable thread entry functions are not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Thread);

