
#pragma once

#include <Foundation/Threading/AtomicUtils.h>

/// \brief Thread-safe reference counting implementation using atomic operations
///
/// Provides the core reference counting functionality that can be used as a base class
/// or component in objects that need reference counting. Uses atomic operations for
/// thread-safe increment/decrement operations. Does not provide automatic deletion -
/// derived classes or users must implement the deletion logic when reference count reaches zero.
class EZ_FOUNDATION_DLL ezRefCountingImpl
{
public:
  /// \brief Constructor
  ezRefCountingImpl() = default;                  // [tested]

  ezRefCountingImpl(const ezRefCountingImpl& rhs) // [tested]
  {
    EZ_IGNORE_UNUSED(rhs);
    // do not copy the ref count
  }

  void operator=(const ezRefCountingImpl& rhs) // [tested]
  {
    EZ_IGNORE_UNUSED(rhs);
    // do not copy the ref count
  }

  /// \brief Increments the reference counter. Returns the new reference count.
  inline ezInt32 AddRef() const // [tested]
  {
    return ezAtomicUtils::Increment(m_iRefCount);
  }

  /// \brief Decrements the reference counter. Returns the new reference count.
  inline ezInt32 ReleaseRef() const // [tested]
  {
    return ezAtomicUtils::Decrement(m_iRefCount);
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
  mutable ezInt32 m_iRefCount = 0; ///< Stores the current reference count
};

/// \brief Base class for objects that require reference counting with virtual destructor
///
/// Extends ezRefCountingImpl with a virtual destructor, making it suitable as a base class
/// for polymorphic objects that need reference counting. Use this when you need virtual
/// function dispatch and proper destruction through base class pointers.
class EZ_FOUNDATION_DLL ezRefCounted : public ezRefCountingImpl
{
public:
  /// \brief Adds a virtual destructor.
  virtual ~ezRefCounted() = default;
};

/// \brief Stores a pointer to a reference counted object and automatically increases / decreases the reference count.
///
/// Note that no automatic deletion etc. happens, this is just to have shared base functionality for reference
/// counted objects. The actual action which, should happen once an object is no longer referenced, obliges
/// to the system that is using the objects.
template <typename T>
class ezScopedRefPointer
{
public:
  /// \brief Constructor.
  ezScopedRefPointer()
    : m_pReferencedObject(nullptr)
  {
  }

  /// \brief Constructor, increases the ref count of the given object.
  ezScopedRefPointer(T* pReferencedObject)
    : m_pReferencedObject(pReferencedObject)
  {
    AddReferenceIfValid();
  }

  ezScopedRefPointer(const ezScopedRefPointer<T>& other)
  {
    m_pReferencedObject = other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// \brief Destructor - releases the reference on the ref-counted object (if there is one).
  ~ezScopedRefPointer() { ReleaseReferenceIfValid(); }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly
  /// assigned object.
  void operator=(T* pNewReference)
  {
    if (pNewReference == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = pNewReference;

    AddReferenceIfValid();
  }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly
  /// assigned object.
  void operator=(const ezScopedRefPointer<T>& other)
  {
    if (other.m_pReferencedObject == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// \brief Returns the referenced object (may be nullptr).
  operator const T*() const { return m_pReferencedObject; }

  /// \brief Returns the referenced object (may be nullptr).
  operator T*() { return m_pReferencedObject; }

  /// \brief Returns the referenced object (may be nullptr).
  const T* operator->() const
  {
    EZ_ASSERT_DEV(m_pReferencedObject != nullptr, "Pointer is nullptr.");
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be nullptr)
  T* operator->()
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


/// \brief Wrapper that makes any type reference counted
///
/// Useful for making existing types reference counted without modifying their implementation.
/// The contained object can be accessed via the m_Content member. Inherits from ezRefCounted
/// to provide virtual destructor and reference counting functionality.
template <typename TYPE>
class ezRefCountedContainer : public ezRefCounted
{
public:
  TYPE m_Content;
};
