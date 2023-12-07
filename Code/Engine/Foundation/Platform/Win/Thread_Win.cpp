#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Threading/Thread.h>

// Thread entry point used to launch ezRunnable instances
DWORD __stdcall ezThreadClassEntryPoint(LPVOID pThreadParameter)
{
  EZ_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  ezThread* pThread = reinterpret_cast<ezThread*>(pThreadParameter);

  return RunThread(pThread);
}

#endif
