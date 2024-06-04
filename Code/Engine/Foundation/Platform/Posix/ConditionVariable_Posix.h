#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Time/Time.h>

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

ezConditionVariable::ezConditionVariable()
{
  pthread_cond_init(&m_Data.m_ConditionVariable, nullptr);
}

ezConditionVariable::~ezConditionVariable()
{
  EZ_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");

  pthread_cond_destroy(&m_Data.m_ConditionVariable);
}

void ezConditionVariable::SignalOne()
{
  pthread_cond_signal(&m_Data.m_ConditionVariable);
}

void ezConditionVariable::SignalAll()
{
  pthread_cond_broadcast(&m_Data.m_ConditionVariable);
}

void ezConditionVariable::UnlockWaitForSignalAndLock() const
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "ezConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  pthread_cond_wait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle());
}

ezConditionVariable::WaitResult ezConditionVariable::UnlockWaitForSignalAndLock(ezTime timeout) const
{
  EZ_ASSERT_DEV(m_iLockCount > 0, "ezConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;

  timeval now;
  gettimeofday(&now, nullptr);

  // pthread_cond_timedwait needs an absolute time value, so compute it from the current time.
  struct timespec timeToWait;

  const ezInt64 iNanoSecondsPerSecond = 1000000000LL;
  const ezInt64 iMicroSecondsPerNanoSecond = 1000LL;

  ezInt64 endTime = now.tv_sec * iNanoSecondsPerSecond + now.tv_usec * iMicroSecondsPerNanoSecond + static_cast<ezInt64>(timeout.GetNanoseconds());

  timeToWait.tv_sec = endTime / iNanoSecondsPerSecond;
  timeToWait.tv_nsec = endTime % iNanoSecondsPerSecond;

  if (pthread_cond_timedwait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle(), &timeToWait) == ETIMEDOUT)
  {
    // inside the lock
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}
