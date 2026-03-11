#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Threading/ConditionVariable.h>
#  include <Foundation/Time/Time.h>

static_assert(sizeof(CONDITION_VARIABLE) == sizeof(ezConditionVariableHandle), "not equal!");

ezConditionVariable::ezConditionVariable()
{
  InitializeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

ezConditionVariable::~ezConditionVariable()
{
  EZ_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");
}

void ezConditionVariable::SignalOne()
{
  WakeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void ezConditionVariable::SignalAll()
{
  WakeAllConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable));
}

void ezConditionVariable::UnlockWaitForSignalAndLock() const
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "ezConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  SleepConditionVariableCS(
    reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable), (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), INFINITE);
}

ezConditionVariable::WaitResult ezConditionVariable::UnlockWaitForSignalAndLock(ezTime timeout) const
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "ezConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;
  DWORD result = SleepConditionVariableCS(reinterpret_cast<CONDITION_VARIABLE*>(&m_Data.m_ConditionVariable),
    (CRITICAL_SECTION*)&m_Mutex.GetMutexHandle(), static_cast<DWORD>(timeout.GetMilliseconds()));

  if (result == WAIT_TIMEOUT)
  {
    // inside the lock at this point
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}

#endif
