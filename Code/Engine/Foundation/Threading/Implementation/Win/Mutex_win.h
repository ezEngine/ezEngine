
#ifdef EZ_MUTEX_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_MUTEX_WIN_INL_H_INCLUDED

inline ezMutex::ezMutex()
{
  InitializeCriticalSection(&m_Handle);
}

inline ezMutex::~ezMutex()
{
  DeleteCriticalSection(&m_Handle);
}

inline void ezMutex::Acquire()
{
  EnterCriticalSection(&m_Handle);
}

inline void ezMutex::Release()
{
  LeaveCriticalSection(&m_Handle);
}

