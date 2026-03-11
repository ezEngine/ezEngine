#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief Provides a simple mechanism for mutual exclusion to prevent multiple threads from accessing a shared resource simultaneously.
///
/// This can be used to protect code that is not thread-safe against race conditions.
/// To ensure that mutexes are always properly released, use the ezLock class or EZ_LOCK macro.
///
/// \sa ezSemaphore, ezConditionVariable
class EZ_FOUNDATION_DLL ezMutex
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMutex);

public:
  ezMutex();
  ~ezMutex();

  /// \brief Acquires an exclusive lock for this mutex object
  void Lock();

  /// \brief Attempts to acquire an exclusive lock for this mutex object. Returns true on success.
  ///
  /// If the mutex is already acquired by another thread, the function returns immediately and returns false.
  ezResult TryLock();

  /// \brief Releases a lock that has been previously acquired
  void Unlock();

  /// \brief Returns true, if the mutex is currently acquired. Can be used to assert that a lock was entered.
  ///
  /// Obviously, this check is not thread-safe and should not be used to check whether a mutex could be locked without blocking.
  /// Use TryLock for that instead.
  EZ_ALWAYS_INLINE bool IsLocked() const { return m_iLockCount > 0; }

  ezMutexHandle& GetMutexHandle() { return m_hHandle; }

private:
  ezMutexHandle m_hHandle;
  ezInt32 m_iLockCount = 0;
};

/// \brief A dummy mutex that does no locking.
///
/// Used when a mutex object needs to be passed to some code (such as allocators), but thread-synchronization
/// is actually not necessary.
class EZ_FOUNDATION_DLL ezNoMutex
{
public:
  /// \brief Implements the 'Acquire' interface function, but does nothing.
  EZ_ALWAYS_INLINE void Lock() {}

  /// \brief Implements the 'TryLock' interface function, but does nothing.
  EZ_ALWAYS_INLINE ezResult TryLock()
  {
    return EZ_SUCCESS;
  }

  /// \brief Implements the 'Release' interface function, but does nothing.
  EZ_ALWAYS_INLINE void Unlock() {}

  EZ_ALWAYS_INLINE bool IsLocked() const
  {
    return false;
  }
};

#include <Mutex_Platform.h>
