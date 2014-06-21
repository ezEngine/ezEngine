#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope. The lock/unlock will only be done if the boolean condition is satisfied at scope creation time.
template <typename T>
class ezConditionalLock
{
public:
  EZ_FORCE_INLINE explicit ezConditionalLock(T& lock, bool bCondition) :
    m_lock(lock), m_bCondition(bCondition)
  {
    if (m_bCondition)
      m_lock.Acquire();
  }

  EZ_FORCE_INLINE ~ezConditionalLock()
  {
    if (m_bCondition)
      m_lock.Release();
  }

private:
  ezConditionalLock();
  ezConditionalLock(const ezConditionalLock<T>& rhs);
  void operator= (const ezConditionalLock<T>& rhs);

  T& m_lock;
  bool m_bCondition;
};

