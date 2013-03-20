
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
  ezRefCounted() : m_iRefCount(0)
  {
  }

  /// \brief Increments the reference counter
  inline void AddRef()
  {
    ezAtomicUtils::Increment(m_iRefCount);
  }

  /// \brief Decrements the reference counter
  inline void ReleaseRef()
  {
    ezAtomicUtils::Decrement(m_iRefCount);
  }

  /// \brief Returns true if the reference count is greater than 0, false otherwise
  inline bool IsReferenced() const
  {
    return m_iRefCount > 0;
  }

  /// \brief Returns the current reference count
  inline ezInt32 GetRefCount() const
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
  ezScopedRefPointer() : m_pReferencedObject(NULL)
  {
  }

  /// \brief Constructor, increases the ref count of the given object.
  ezScopedRefPointer(T* pReferencedObject) : m_pReferencedObject(pReferencedObject)
  {
    AddReferenceIfValid();
  }

  /// \brief Destructor - releases the reference on the refcounted object (if there is one).
  ~ezScopedRefPointer()
  {
    ReleaseReferenceIfValid();
  }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly assigned object.
  void operator = (T* pNewReference)
  {
    if(pNewReference == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = pNewReference;

    AddReferenceIfValid();
  }

  /// \brief Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly assigned object.
  void operator = (const ezScopedRefPointer<T>& Other)
  {
    if(Other.m_pReferencedObject == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = Other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// \brief Returns the referenced object (may be NULL).
  operator const T*() const
  {
    EZ_ASSERT(m_pReferencedObject != NULL, "Pointer is NULL.");
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be NULL).
  operator T*()
  {
    EZ_ASSERT(m_pReferencedObject != NULL, "Pointer is NULL.");
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be NULL).
  const T* operator ->() const
  {
    EZ_ASSERT(m_pReferencedObject != NULL, "Pointer is NULL.");
    return m_pReferencedObject;
  }

  /// \brief Returns the referenced object (may be NULL)
  T* operator -> ()
  {
    EZ_ASSERT(m_pReferencedObject != NULL, "Pointer is NULL.");
    return m_pReferencedObject;
  }

private:

  /// \brief Internal helper function to add a reference on the current object (if != NULL)
  inline void AddReferenceIfValid()
  {
    if(m_pReferencedObject != NULL)
    {
      m_pReferencedObject->AddRef();
    }
  }

  /// \brief Internal helper function to release a reference on the current object (if != NULL)
  inline void ReleaseReferenceIfValid()
  {
    if(m_pReferencedObject != NULL)
    {
      m_pReferencedObject->ReleaseRef();
    }
  }

  T* m_pReferencedObject; ///< Stores a pointer to the referenced object
};


