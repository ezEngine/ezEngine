
#pragma once

#include <Foundation/Threading/AtomicUtils.h>

/// Base class for reference counted objects
/// Note that no automatic deletion etc. happens, this is just to have shared base functionality for reference
/// counted objects - the actual actions which should happen once an object is no longer referenced obliges
/// to the actual system using the objects.
class EZ_FOUNDATION_DLL ezRefCounted
{
public:

  /// Constructor
  ezRefCounted()
    : m_iRefCount(0)
  {
  }

  /// Increments the reference counter
  inline void AddRef()
  {
    ezAtomicUtils::Increment(m_iRefCount);
  }

  /// Decrements the reference counter
  inline void ReleaseRef()
  {
    ezAtomicUtils::Decrement(m_iRefCount);
  }

  /// Returns true if the reference count is greater than 0, false otherwise
  inline bool IsReferenced() const
  {
    return m_iRefCount > 0;
  }

  /// Returns the current reference count
  inline ezInt32 GetRefCount() const
  {
    return m_iRefCount;
  }

private:

  ezInt32 m_iRefCount; ///< Stores the current reference count
};

/// Stores a pointer to a reference counted object and automatically increases / decreases
/// the ref count.
template <typename T> 
class ezScopedRefPointer
{
public:

  /// Constructor
  ezScopedRefPointer()
    : m_pReferencedObject(NULL)
  {
  }

  /// Constructor
  ezScopedRefPointer(T* pReferencedObject)
    : m_pReferencedObject(pReferencedObject)
  {
    AddReferenceIfValid();
  }

  /// Destructor - releases the reference on the refcounted object (if there is one)
  ~ezScopedRefPointer()
  {
    ReleaseReferenceIfValid();
  }

  /// Assignment operator
  void operator = (T* pNewReference)
  {
    if(pNewReference == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = pNewReference;

    AddReferenceIfValid();
  }

  /// Assignment operator
  void operator = (const ezScopedRefPointer<T>& Other)
  {
    if(Other.m_pReferencedObject == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = Other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// Returns the referenced object (may be NULL)
  operator const T*() const
  {
    return m_pReferencedObject;
  }

  /// Returns the referenced object (may be NULL)
  operator T*()
  {
    return m_pReferencedObject;
  }

  /// Returns the referenced object (may be NULL)
  const T* operator ->() const
  {
    return m_pReferencedObject;
  }

  /// Returns the referenced object (may be NULL)
  T* operator -> ()
  {
    return m_pReferencedObject;
  }

private:

  /// Internal helper function to add a reference on the current object (if != NULL)
  inline void AddReferenceIfValid()
  {
    if(m_pReferencedObject != NULL)
    {
      m_pReferencedObject->AddRef();
    }
  }

  /// Internal helper function to release a reference on the current object (if != NULL)
  inline void ReleaseReferenceIfValid()
  {
    if(m_pReferencedObject != NULL)
    {
      m_pReferencedObject->ReleaseRef();
    }
  }

  T* m_pReferencedObject; ///< Stores a pointer to the referenced object
};


