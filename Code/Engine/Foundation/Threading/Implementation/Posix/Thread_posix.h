
#ifdef EZ_THREAD_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREAD_POSIX_INL_H_INCLUDED

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch ezRunnable instances
void* ezThreadClassEntryPoint(void* pThreadParameter)
{
  EZ_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  ezThread* pThread = reinterpret_cast<ezThread*>(pThreadParameter);
  
  RunThread(pThread);
  
  return nullptr;
}

/// \endcond

