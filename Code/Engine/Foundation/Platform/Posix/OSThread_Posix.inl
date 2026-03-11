#include <Foundation/Threading/Implementation/OSThread.h>
#include <Foundation/Threading/Thread.h>

ezAtomicInteger32 ezOSThread::s_iThreadCount;

// Posix specific implementation of the thread class

ezOSThread::ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData /*= nullptr*/, ezStringView sName /*= "ezThread"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
{
  s_iThreadCount.Increment();

  m_EntryPoint = pThreadEntryPoint;
  m_pUserData = pUserData;
  m_sName = sName;
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

  int iReturnCode = pthread_create(&m_hHandle, &ThreadAttributes, m_EntryPoint, m_pUserData);
  EZ_IGNORE_UNUSED(iReturnCode);
  EZ_ASSERT_RELEASE(iReturnCode == 0, "Thread creation failed!");

#ifdef EZ_POSIX_THREAD_SETNAME
  if (iReturnCode == 0 && !m_sName.IsEmpty())
  {
    // pthread has a thread name limit of 16 bytes.
    // This means 15 characters and the terminating '\0'
    if (m_sName.GetElementCount() < 16)
    {
      pthread_setname_np(m_hHandle, m_sName.GetData());
    }
    else
    {
      char threadName[16];
      strncpy(threadName, m_sName.GetData(), 15);
      threadName[15] = '\0';
      pthread_setname_np(m_hHandle, threadName);
    }
  }
#endif

  m_ThreadID = m_hHandle;

  pthread_attr_destroy(&ThreadAttributes);
}

/// Joins with the thread (waits for termination)
void ezOSThread::Join()
{
  pthread_join(m_hHandle, nullptr);
}
