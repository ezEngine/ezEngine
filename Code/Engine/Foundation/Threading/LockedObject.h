#pragma once

/// \brief Provides access to an object while managing a lock (e.g. a mutex) that ensures that during its lifetime the access to the object
/// happens under the lock.
template <typename T, typename O>
class ezLockedObject
{
public:
  EZ_ALWAYS_INLINE explicit ezLockedObject(T& lock, O* pObject)
      : m_pLock(&lock)
      , m_pObject(pObject)
  {
    m_pLock->Acquire();
  }
  ezLockedObject() = default;

  EZ_ALWAYS_INLINE ezLockedObject(ezLockedObject<T, O>&& rhs) { *this = std::move(rhs); }

  ezLockedObject(const ezLockedObject<T, O>& rhs) = delete;
  
  void operator=(const ezLockedObject<T, O>&& rhs)
  {
    if (m_pLock)
      m_pLock->Release();
    m_pLock = rhs.m_pLock;
    rhs.m_pLock = nullptr;
    m_pObject = rhs.m_pObject;
    rhs.m_pObject = nullptr;
  }

  void operator=(const ezLockedObject<T, O>& rhs) = delete;

  EZ_ALWAYS_INLINE ~ezLockedObject()
  {
    if (m_pLock)
      m_pLock->Release();
  }

  /// \brief Whether the encapsulated object exists at all or is nullptr
  EZ_ALWAYS_INLINE bool isValid() const { return m_pObject != nullptr; }

  O* operator->() { return m_pObject; }

  const O* operator->() const { return m_pObject; }

  O& operator*() { return *m_pObject; }

  const O& operator*() const { return *m_pObject; }

  bool operator==(const O* rhs) const { return m_pObject == rhs; }

  bool operator!=(const O* rhs) const { return m_pObject != rhs; }

  bool operator!() const { return m_pObject == nullptr; }

  operator bool() const { return m_pObject != nullptr; }

private:
  mutable T* m_pLock = nullptr;
  mutable O* m_pObject = nullptr;
};

