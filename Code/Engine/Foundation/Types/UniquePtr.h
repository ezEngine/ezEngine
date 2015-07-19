#pragma once

#include <Foundation/Basics.h>

/// \brief A Unique ptr manages an object and destroys that object when it goes out of scope. It is ensure that only one unique ptr can manage the same object.
template <typename T>
class ezUniquePtr
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();
  
  /// \brief Creates an empty unique ptr.
  ezUniquePtr();

  /// \brief Creates a unique ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  ezUniquePtr(const ezInternal::NewInstance<U>& instance);

  /// \brief Creates a unique ptr from a pointer and an allocator. The passed allocator will be used to destroy the instance when the unique ptr goes out of scope.
  template <typename U>
  ezUniquePtr(U* pInstance, ezAllocatorBase* pAllocator);

  /// \brief Move constructs a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one unique ptr managing the same object.
  template <typename U>
  ezUniquePtr(ezUniquePtr<U>&& other);

  /// \brief Destroys the managed object using the stored allocator.
  ~ezUniquePtr();

  /// \brief Sets the unique ptr from a freshly created instance through EZ_NEW or EZ_DEFAULT_NEW.
  template <typename U>
  void operator=(const ezInternal::NewInstance<U>& instance);

  /// \brief Move assigns a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one unique ptr managing the same object.
  template <typename U>
  void operator=(ezUniquePtr<U>&& other);

  /// \brief Releases the managed object. The unique ptr will be empty afterwards.
  T* Release();

  /// \brief Borrows the managed object. The unique ptr stays unmodified.
  T* Borrow() const;

  /// \brief Destroys the managed object and resets the unique ptr.
  void Reset();

  /// \brief Provides access to the managed object.
  T& operator*() const;

  /// \brief Provides access to the managed object.
  T* operator->() const;

  /// \brief Returns true if there is managed object and false if the unique ptr is empty.
  operator bool() const;

  /// \brief Compares the unique ptr against another unique ptr.
  bool operator==(const ezUniquePtr<T>& rhs) const;
  bool operator!=(const ezUniquePtr<T>& rhs) const;
  bool operator<(const ezUniquePtr<T>& rhs) const;
  bool operator<=(const ezUniquePtr<T>& rhs) const;
  bool operator>(const ezUniquePtr<T>& rhs) const;
  bool operator>=(const ezUniquePtr<T>& rhs) const;

  /// \brief Compares the unique ptr against nullptr.
  bool operator==(std::nullptr_t) const;
  bool operator!=(std::nullptr_t) const;
  bool operator<(std::nullptr_t) const;
  bool operator<=(std::nullptr_t) const;
  bool operator>(std::nullptr_t) const;
  bool operator>=(std::nullptr_t) const;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezUniquePtr);

private:
  template<typename U>
  friend class ezUniquePtr;

  T* m_pInstance;
  ezAllocatorBase* m_pAllocator;
};

#include <Foundation/Types/Implementation/UniquePtr_inl.h>
