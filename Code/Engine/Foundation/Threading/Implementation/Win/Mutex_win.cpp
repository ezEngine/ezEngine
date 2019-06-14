#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/Threading/Mutex.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

template <ezUInt32 a, ezUInt32 b>
struct SameSize
{
  static_assert(a == b, "Critical section has incorrect size");
};

template <ezUInt32 a, ezUInt32 b>
struct SameAlignment
{
  static_assert(a == b, "Critical section has incorrect alignment");
};


ezMutex::ezMutex()
{
  SameSize<sizeof(ezMutexHandle), sizeof(CRITICAL_SECTION)> check1; (void)check1;
  SameAlignment<alignof(ezMutexHandle), alignof(CRITICAL_SECTION)> check2; (void)check2;
  InitializeCriticalSection((CRITICAL_SECTION*)&m_Handle);
}

ezMutex::~ezMutex()
{
  DeleteCriticalSection((CRITICAL_SECTION*)&m_Handle);
}
#endif
