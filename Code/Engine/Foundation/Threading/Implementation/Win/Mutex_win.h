#ifdef EZ_MUTEX_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define EZ_MUTEX_WIN_INL_H_INCLUDED

#if EZ_ENABLED(EZ_COMPILER_MSVC) && EZ_ENABLED(EZ_PLATFORM_ARCH_X86)

extern "C"
{
  // The main purpose of this little hack here is to have Mutex::Lock and Mutex::Unlock inline-able without including windows.h
  // The hack however does only work on the MSVC compiler. See fall back code below.

  // First define two functions which are binary compatible with EnterCriticalSection and LeaveCriticalSection
  __declspec(dllimport) void __stdcall ezWinEnterCriticalSection(ezMutexHandle* handle);
  __declspec(dllimport) void __stdcall ezWinLeaveCriticalSection(ezMutexHandle* handle);
  __declspec(dllimport) ezMinWindows::BOOL __stdcall ezWinTryEnterCriticalSection(ezMutexHandle* handle);

  // Now redirect them through linker flags to the correct implementation
#  if EZ_ENABLED(EZ_PLATFORM_32BIT)
#    pragma comment(linker, "/alternatename:__imp__ezWinEnterCriticalSection@4=__imp__EnterCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__ezWinLeaveCriticalSection@4=__imp__LeaveCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__ezWinTryEnterCriticalSection@4=__imp__TryEnterCriticalSection@4")
#  else
#    pragma comment(linker, "/alternatename:__imp_ezWinEnterCriticalSection=__imp_EnterCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_ezWinLeaveCriticalSection=__imp_LeaveCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_ezWinTryEnterCriticalSection=__imp_TryEnterCriticalSection")
#  endif
}

inline void ezMutex::Lock()
{
  ezWinEnterCriticalSection(&m_hHandle);
  ++m_iLockCount;
}

inline void ezMutex::Unlock()
{
  --m_iLockCount;
  ezWinLeaveCriticalSection(&m_hHandle);
}

inline ezResult ezMutex::TryLock()
{
  if (ezWinTryEnterCriticalSection(&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

#else

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

inline void ezMutex::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)&m_hHandle);
  ++m_iLockCount;
}

inline void ezMutex::Unlock()
{
  --m_iLockCount;
  LeaveCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

inline ezResult ezMutex::TryLock()
{
  if (TryEnterCriticalSection((CRITICAL_SECTION*)&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

#endif
