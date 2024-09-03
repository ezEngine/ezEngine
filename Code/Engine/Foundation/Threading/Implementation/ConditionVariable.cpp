#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

void ezConditionVariable::Lock()
{
  m_Mutex.Lock();
  ++m_iLockCount;
}

ezResult ezConditionVariable::TryLock()
{
  if (m_Mutex.TryLock().Succeeded())
  {
    ++m_iLockCount;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezConditionVariable::Unlock()
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}


