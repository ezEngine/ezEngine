
#pragma once

#include <Foundation/Threading/AtomicUtils.h>

/// \brief Base class for reference counted objects.
///
/// Note that no automatic deletion etc. happens, this is just to have shared base functionality for reference
/// counted objects. The actual action which, should happen once an object is no longer referenced, obliges
/// to the system that is using the objects.
class EZ_FOUNDATION_DLL ezRefCounted
{
public:

  /// \brief Constructor
  ezRefCounted() : m_iRefCount(0) // [tested]
  {
  }

  ezRefCounted(const ezRefCounted& rhs) : m_iRefCount(0) // [tested]
  {
    // do not copy the ref count
  }

  void operator=(const ezRefCounted& rhs) // [tested]
  {
    // do not copy the ref count
  }

  /// \brief Increments the reference counter
  inline void AddRef() // [tested]
  {
    ezAtomicUtils::Increment(m_iRefCount);
  }

  /// \brief Decrements the reference counter
  inline void ReleaseRef() // [tested]
  {
    ezAtomicUtils::Decrement(m_iRefCount);
  }

  /// \brief Returns true if the reference count is greater than 0, false otherwise
  inline bool IsReferenced() const // [tested]
  {
    return m_iRefCount > 0;
  }

  /// \brief Returns the current reference count
  inline ezInt32 GetRefCount() const // [tested]
  {
    return m_iRefCount;
  }

private:

  ezInt32 m_iRefCount; ///< Stores the current reference count
};

/// \brief Stores a pointer to a reference counted object and automatically increases / decreases the reference count.
template <typename T> 
class ezScopedRefPointer
{
public:

  /// \brief Constructor.
  ezScopedRefPointer() : m_pReferencedObject(nullptr)
  {
  }

  /// \brief Constructor, increases the ref count of the given object.
  ezScopedRefPointer(T* pReferencedObject) : m_pReferencedObject(pReferencedObject)
  {
    AddReferenceIfValid();
  }

  ezScopedRefPointer(const ezScopedRefPointer<T>& Other)
  {
    m_pReferencedObject = Other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// \brief Destructor - releases the reference on the ref-counted object (if there is one).
  ~ezScopedRefPointer()
  {
    ReleaseReferenceIfValid();
  }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly assigned object.
  void operator = (T* pNewReference)
  {
    if (pNewReference == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = pNewReference;

    AddReferenceIfValid();
  }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly assigned object.
  void operator = (const ezScopedRefPointer<T>& Other)
  {
    if (Other.m_pReferencedObject == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = Other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// \brief Returns the referenced object (may be nullptr).
  operator const T*() const
  {
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be nullptr).
  operator T*()
  {
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be nullptr).
  const T* operator ->() const
  {
    EZ_ASSERT_DEV(m_pReferencedObject != nullptr, "Pointer is nullptr.");
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be nullptr)
  T* operator -> ()
  {
    EZ_ASSERT_DEV(m_pReferencedObject != nullptr, "Pointer is nullptr.");
    return m_pReferencedObject;
  }

private:

  /// \brief Internal helper function to add a reference on the current object (if != nullptr)
  inline void AddReferenceIfValid()
  {
    if (m_pReferencedObject != nullptr)
    {
      m_pReferencedObject->AddRef();
    }
  }

  /// \brief Internal helper function to release a reference on the current object (if != nullptr)
  inline void ReleaseReferenceIfValid()
  {
    if (m_pReferencedObject != nullptr)
    {
      m_pReferencedObject->ReleaseRef();
    }
  }

  T* m_pReferencedObject; ///< Stores a pointer to the referenced object
};


