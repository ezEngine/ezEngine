#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

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

