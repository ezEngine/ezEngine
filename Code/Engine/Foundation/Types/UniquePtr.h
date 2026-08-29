#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>

/// A Unique ptr manages an object and destroys that object when it goes out of scope. It is ensure that only one unique ptr can
/// manage the same object.
template <typename T>
class ezUniquePtr
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezUniquePtr);

public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Creates an empty unique ptr.
  ezUniquePtr();

  /// Creates a unique ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  ezUniquePtr(const ezInternal::NewInstance<U>& instance);

  /// Creates a unique ptr from a pointer and an allocator. The passed allocator will be used to destroy the instance when the unique
  /// ptr goes out of scope.
  template <typename U>
  ezUniquePtr(U* pInstance, ezAllocator* pAllocator);

  /// Move constructs a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one
  /// unique ptr managing the same object.
  template <typename U>
  ezUniquePtr(ezUniquePtr<U>&& other);

  /// Initialization with nullptr to be able to return nullptr in functions that return unique ptr.
  ezUniquePtr(std::nullptr_t);

  /// Destroys the managed object using the stored allocator.
  ~ezUniquePtr();

  /// Sets the unique ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  ezUniquePtr<T>& operator=(const ezInternal::NewInstance<U>& instance);

  /// Move assigns a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one
  /// unique ptr managing the same object.
  template <typename U>
  ezUniquePtr<T>& operator=(ezUniquePtr<U>&& other);

  /// Same as calling 'Reset()'
  ezUniquePtr<T>& operator=(std::nullptr_t);

  /// Releases the managed object without destroying it. The unique ptr will be empty afterwards.
  T* Release();

  /// Releases the managed object without destroying it. The unique ptr will be empty afterwards. Also returns the allocator that
  /// should be used to destroy the object.
  T* Release(ezAllocator*& out_pAllocator);

  /// Borrows the managed object. The unique ptr stays unmodified.
  T* Borrow() const;

  /// Destroys the managed object and resets the unique ptr.
  void Clear();

  /// Provides access to the managed object.
  T& operator*() const;

  /// Provides access to the managed object.
  T* operator->() const;

  /// Returns true if there is managed object and false if the unique ptr is empty.
  explicit operator bool() const;

  /// Compares the unique ptr against another unique ptr.
  bool operator==(const ezUniquePtr<T>& rhs) const;
  bool operator!=(const ezUniquePtr<T>& rhs) const;
  bool operator<(const ezUniquePtr<T>& rhs) const;
  bool operator<=(const ezUniquePtr<T>& rhs) const;
  bool operator>(const ezUniquePtr<T>& rhs) const;
  bool operator>=(const ezUniquePtr<T>& rhs) const;

  /// Compares the unique ptr against nullptr.
  bool operator==(std::nullptr_t) const;
  bool operator!=(std::nullptr_t) const;
  bool operator<(std::nullptr_t) const;
  bool operator<=(std::nullptr_t) const;
  bool operator>(std::nullptr_t) const;
  bool operator>=(std::nullptr_t) const;

private:
  template <typename U>
  friend class ezUniquePtr;

  T* m_pInstance = nullptr;
  ezAllocator* m_pAllocator = nullptr;
};

#include <Foundation/Types/Implementation/UniquePtr_inl.h>
