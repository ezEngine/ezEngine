#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
template <typename T>
class ezLock
{
public:
  EZ_FORCE_INLINE explicit ezLock(T& lock) :
    m_lock(lock)
  {
    m_lock.Acquire();
  }

  EZ_FORCE_INLINE ~ezLock()
  {
    m_lock.Release();
  }

private:
  ezLock();
  ezLock(const ezLock<T>& rhs);
  void operator= (const ezLock<T>& rhs);

  T& m_lock;
};

/// \brief Shortcut for ezLock<Type> l(lock)
#define EZ_LOCK(lock) ezLock<decltype(lock)> EZ_CONCAT(l_, EZ_SOURCE_LINE)(lock)

