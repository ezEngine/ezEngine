#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
template <typename T>
class ezLock
{
public:
  EZ_ALWAYS_INLINE explicit ezLock(T& ref_lock)
    : m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  EZ_ALWAYS_INLINE ~ezLock() { m_Lock.Unlock(); }

private:
  ezLock();
  ezLock(const ezLock<T>& rhs);
  void operator=(const ezLock<T>& rhs);

  T& m_Lock;
};

/// \brief Shortcut for ezLock<Type> l(lock)
#define EZ_LOCK(lock) ezLock<decltype(lock)> EZ_PP_CONCAT(l_, EZ_SOURCE_LINE)(lock)
