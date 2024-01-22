#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief A Shared ptr manages a shared object and destroys that object when no one references it anymore. The managed object must derive
/// from ezRefCounted.
template <typename T>
class ezSharedPtr
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Creates an empty shared ptr.
  ezSharedPtr();

  /// \brief Creates a shared ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  ezSharedPtr(const ezInternal::NewInstance<U>& instance);

  /// \brief Creates a shared ptr from a pointer and an allocator. The passed allocator will be used to destroy the instance when the shared
  /// ptr goes out of scope.
  template <typename U>
  ezSharedPtr(U* pInstance, ezAllocator* pAllocator);

  /// \brief Copy constructs a shared ptr from another. Both will hold a reference to the managed object afterwards.
  ezSharedPtr(const ezSharedPtr<T>& other);

  /// \brief Copy constructs a shared ptr from another. Both will hold a reference to the managed object afterwards.
  template <typename U>
  ezSharedPtr(const ezSharedPtr<U>& other);

  /// \brief Move constructs a shared ptr from another. The other shared ptr will be empty afterwards.
  template <typename U>
  ezSharedPtr(ezSharedPtr<U>&& other);

  /// \brief Move constructs a shared ptr from a unique ptr. The unique ptr will be empty afterwards.
  template <typename U>
  ezSharedPtr(ezUniquePtr<U>&& other);

  /// \brief Initialization with nullptr to be able to return nullptr in functions that return shared ptr.
  ezSharedPtr(std::nullptr_t);

  /// \brief Destroys the managed object using the stored allocator if no one else references it anymore.
  ~ezSharedPtr();

  /// \brief Sets the shared ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  ezSharedPtr<T>& operator=(const ezInternal::NewInstance<U>& instance);

  /// \brief Sets the shared ptr from another. Both will hold a reference to the managed object afterwards.
  ezSharedPtr<T>& operator=(const ezSharedPtr<T>& other);

  /// \brief Sets the shared ptr from another. Both will hold a reference to the managed object afterwards.
  template <typename U>
  ezSharedPtr<T>& operator=(const ezSharedPtr<U>& other);

  /// \brief Move assigns a shared ptr from another. The other shared ptr will be empty afterwards.
  template <typename U>
  ezSharedPtr<T>& operator=(ezSharedPtr<U>&& other);

  /// \brief Move assigns a shared ptr from a unique ptr. The unique ptr will be empty afterwards.
  template <typename U>
  ezSharedPtr<T>& operator=(ezUniquePtr<U>&& other);

  /// \brief Assigns a nullptr to the shared ptr. Same as Reset.
  ezSharedPtr<T>& operator=(std::nullptr_t);

  /// \brief Borrows the managed object. The shared ptr stays unmodified.
  T* Borrow() const;

  /// \brief Destroys the managed object if no one else references it anymore and resets the shared ptr.
  void Clear();

  /// \brief Provides access to the managed object.
  T& operator*() const;

  /// \brief Provides access to the managed object.
  T* operator->() const;

  /// \brief Provides access to the managed object.
  operator const T*() const;

  /// \brief Provides access to the managed object.
  operator T*();

  /// \brief Returns true if there is managed object and false if the shared ptr is empty.
  explicit operator bool() const;

  /// \brief Compares the shared ptr against another shared ptr.
  bool operator==(const ezSharedPtr<T>& rhs) const;
  bool operator!=(const ezSharedPtr<T>& rhs) const;
  bool operator<(const ezSharedPtr<T>& rhs) const;
  bool operator<=(const ezSharedPtr<T>& rhs) const;
  bool operator>(const ezSharedPtr<T>& rhs) const;
  bool operator>=(const ezSharedPtr<T>& rhs) const;

  /// \brief Compares the shared ptr against nullptr.
  bool operator==(std::nullptr_t) const;
  bool operator!=(std::nullptr_t) const;
  bool operator<(std::nullptr_t) const;
  bool operator<=(std::nullptr_t) const;
  bool operator>(std::nullptr_t) const;
  bool operator>=(std::nullptr_t) const;

  /// \brief Returns a copy of this, as an ezSharedPtr<DERIVED>. Downcasts the stored pointer (using static_cast).
  ///
  /// Does not check whether the cast would be valid, that is all your responsibility.
  template <typename DERIVED>
  ezSharedPtr<DERIVED> Downcast() const
  {
    return ezSharedPtr<DERIVED>(static_cast<DERIVED*>(m_pInstance), m_pAllocator);
  }

private:
  template <typename U>
  friend class ezSharedPtr;

  void AddReferenceIfValid();
  void ReleaseReferenceIfValid();

  T* m_pInstance;
  ezAllocator* m_pAllocator;
};

#include <Foundation/Types/Implementation/SharedPtr_inl.h>
