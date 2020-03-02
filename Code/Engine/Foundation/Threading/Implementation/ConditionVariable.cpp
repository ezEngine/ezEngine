#include <FoundationPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

void ezConditionVariable::Lock()
{
  m_Mutex.Lock();
  ++m_iLockCount;
}

bool ezConditionVariable::TryLock()
{
  if (m_Mutex.TryLock())
  {
    ++m_iLockCount;
    return true;
  }

  return false;
}

void ezConditionVariable::Unlock()
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/ConditionVariable_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/ConditionVariable_posix.h>
#else
#  error "Unsupported Platform."
#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadSignal);
