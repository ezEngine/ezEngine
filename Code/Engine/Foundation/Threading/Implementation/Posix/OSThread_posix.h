
#ifdef EZ_OSTHREAD_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_OSTHREAD_POSIX_INL_H_INCLUDED


// Windows specific implementation of the thread class

ezOSThread::ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData /*= NULL*/, const char* szName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  pthread_attr_t ThreadAttributes;
  pthread_attr_init(&ThreadAttributes);
  pthread_attr_setdetachstate(&ThreadAttributes, PTHREAD_CREATE_JOINABLE);
  
  int iReturnCode = pthread_create(&m_Handle, &ThreadAttributes, pThreadEntryPoint, pUserData);
  EZ_ASSERT(iReturnCode == 0, "Thread creation failed!");
  
  pthread_attr_destroy(&ThreadAttributes);
  
  m_EntryPoint = pThreadEntryPoint;  
}

ezOSThread::~ezOSThread()
{
}

/// Attempts to acquire an exclusive lock for this mutex object
void ezOSThread::Start()
{
  /// \todo Complicated
  
}

/// Releases a lock that has been previously acquired
void ezOSThread::Join()
{
  pthread_join(m_Handle, NULL);
}

