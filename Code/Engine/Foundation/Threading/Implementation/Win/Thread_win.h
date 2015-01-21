
#ifdef EZ_THREAD_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREAD_WIN_INL_H_INCLUDED

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch ezRunnable instances
DWORD __stdcall ezThreadClassEntryPoint(LPVOID lpThreadParameter)
{
  EZ_ASSERT_RELEASE(lpThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  ezThread* pThread = reinterpret_cast<ezThread*>(lpThreadParameter);
  
  return RunThread(pThread);
}


/// \endcond

