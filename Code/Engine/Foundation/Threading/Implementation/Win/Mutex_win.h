#ifdef EZ_MUTEX_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define EZ_MUTEX_WIN_INL_H_INCLUDED

#ifdef _MSC_VER

extern "C"
{
  // The main purpose of this little hack here is to have Mutex::Acquire and Mutex::Release inline-able without including windows.h
  // The hack however does only work on the MSVC compiler. See fall back code below.

  // First define two functions which are binary compatible with EnterCriticalSection and LeaveCriticalSection
  __declspec(dllimport) void __stdcall ezWinEnterCriticalSection(ezMutexHandle* handle);
  __declspec(dllimport) void __stdcall ezWinLeaveCriticalSection(ezMutexHandle* handle);

  // Now redirect them through linker flags to the correct implementation
#  if EZ_ENABLED(EZ_PLATFORM_32BIT)
#    pragma comment(linker, "/alternatename:__imp__ezWinEnterCriticalSection@4=__imp__EnterCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__ezWinLeaveCriticalSection@4=__imp__LeaveCriticalSection@4")
#  else
#    pragma comment(linker, "/alternatename:__imp_ezWinEnterCriticalSection=__imp_EnterCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_ezWinLeaveCriticalSection=__imp_LeaveCriticalSection")
#  endif
}

inline void ezMutex::Acquire()
{
  ezWinEnterCriticalSection(&m_Handle);
  ++m_iLockCount;
}

inline void ezMutex::Release()
{
  --m_iLockCount;
  ezWinLeaveCriticalSection(&m_Handle);
}

#else

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

inline void ezMutex::Acquire()
{
  EnterCriticalSection((CRITICAL_SECTION*)&m_Handle);
  ++m_iLockCount;
}

inline void ezMutex::Release()
{
  --m_iLockCount;
  LeaveCriticalSection((CRITICAL_SECTION*)&m_Handle);
}
#endif
