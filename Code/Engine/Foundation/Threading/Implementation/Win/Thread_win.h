
#ifdef EZ_THREAD_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREAD_WIN_INL_H_INCLUDED

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch ezRunnable instances
DWORD __stdcall ezThreadClassEntryPoint(LPVOID lpThreadParameter)
{
   EZ_ASSERT(lpThreadParameter != NULL, "thread parameter in thread entry point must not be NULL!");

  ezThread* pThread = reinterpret_cast<ezThread*>(lpThreadParameter);
  
  if(pThread == NULL)
    return 0;

  ezThreadLocalStorage::SetPerThreadPointerTable(&(pThread->m_ThreadLocalPointerTable));

  pThread->m_ThreadStatus = ezThread::Running;

  // Run the worker thread function
  ezUInt32 uiReturnCode = pThread->Run();

  pThread->m_ThreadStatus = ezThread::Finished;

  return uiReturnCode;
}


/// \endcond