
#ifdef EZ_THREAD_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREAD_POSIX_INL_H_INCLUDED

// Thread entry point used to launch ezRunnable instances
void* ezThreadClassEntryPoint(void* pThreadParameter)
{
   EZ_ASSERT(pThreadParameter != NULL, "thread parameter in thread entry point must not be NULL!");

  ezThread* pThread = reinterpret_cast<ezThread*>(pThreadParameter);
  
  if(pThread == NULL)
    return NULL;

  ezThreadLocalStorage::SetPerThreadPointerTable(&(pThread->m_ThreadLocalPointerTable));

  pThread->m_ThreadStatus = ezThread::Running;

  // Run the worker thread function
  pThread->Run();

  pThread->m_ThreadStatus = ezThread::Finished;
  
  return NULL;
}