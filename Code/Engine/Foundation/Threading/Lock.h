#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
/// Works with any object that implements Lock() and Unlock() methods (ezMutex, etc.).
/// Use the EZ_LOCK macro for convenient scoped locking.
template <typename T>
class ezLock
{
public:
  /// \brief Acquires the lock immediately upon construction
  EZ_ALWAYS_INLINE explicit ezLock(T& ref_lock)
    : m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  /// \brief Automatically releases the lock when the object is destroyed
  EZ_ALWAYS_INLINE ~ezLock() { m_Lock.Unlock(); }

private:
  ezLock();
  ezLock(const ezLock<T>& rhs);
  void operator=(const ezLock<T>& rhs);

  T& m_Lock;
};

/// \brief Convenient macro for creating a scoped lock with automatic type deduction
///
/// Creates an ezLock instance with a unique name based on the source line number.
/// The lock is held for the duration of the current scope. Equivalent to:
/// ezLock<decltype(lock)> variable_name(lock);
///
/// Example: EZ_LOCK(myMutex); // Locks myMutex until end of scope
#define EZ_LOCK(lock) ezLock<decltype(lock)> EZ_PP_CONCAT(l_, EZ_SOURCE_LINE)(lock)
