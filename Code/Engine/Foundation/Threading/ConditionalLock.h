#pragma once

/// \brief RAII lock guard that conditionally acquires and releases locks based on runtime conditions
///
/// Provides the same automatic lock management as ezLock but only performs the actual locking
/// when a boolean condition is met. Useful in scenarios where locking is only required under
/// certain circumstances, avoiding unnecessary synchronization overhead when protection is not needed.
///
/// The condition is evaluated once at construction time. If false, no locking occurs throughout
/// the object's lifetime, making this essentially a no-op with zero runtime cost.
template <typename T>
class ezConditionalLock
{
public:
  EZ_ALWAYS_INLINE explicit ezConditionalLock(T& lock, bool bCondition)
    : m_lock(lock)
    , m_bCondition(bCondition)
  {
    if (m_bCondition)
    {
      m_lock.Lock();
    }
  }

  EZ_ALWAYS_INLINE ~ezConditionalLock()
  {
    if (m_bCondition)
    {
      m_lock.Unlock();
    }
  }

private:
  ezConditionalLock();
  ezConditionalLock(const ezConditionalLock<T>& rhs);
  void operator=(const ezConditionalLock<T>& rhs);

  T& m_lock;
  bool m_bCondition;
};
