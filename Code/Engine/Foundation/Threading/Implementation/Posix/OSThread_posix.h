
#ifdef EZ_OSTHREAD_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_OSTHREAD_POSIX_INL_H_INCLUDED

ezAtomicInteger32 ezOSThread::s_iThreadCount;

// Posix specific implementation of the thread class

ezOSThread::ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData /*= nullptr*/, const char* szName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  s_iThreadCount.Increment();

  m_EntryPoint = pThreadEntryPoint;
  m_pUserData = pUserData;
  m_szName = szName;
  m_uiStackSize = uiStackSize;
  
  // Thread creation is deferred since Posix threads can't be created sleeping
}

ezOSThread::~ezOSThread()
{
  s_iThreadCount.Decrement();
}

/// Starts the thread
void ezOSThread::Start()
{
  pthread_attr_t ThreadAttributes;
  pthread_attr_init(&ThreadAttributes);
  pthread_attr_setdetachstate(&ThreadAttributes, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setstacksize(&ThreadAttributes, m_uiStackSize);
  
  int iReturnCode = pthread_create(&m_Handle, &ThreadAttributes, m_EntryPoint, m_pUserData);
  EZ_IGNORE_UNUSED(iReturnCode);
  EZ_ASSERT_RELEASE(iReturnCode == 0, "Thread creation failed!");

  m_ThreadID = m_Handle;
  
  pthread_attr_destroy(&ThreadAttributes);
}

/// Joins with the thread (waits for termination)
void ezOSThread::Join()
{
  pthread_join(m_Handle, nullptr);
}

