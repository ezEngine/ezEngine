#pragma once

#include <Foundation/Basics.h>

/// \brief
template <typename T>
class ezUniquePtr
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();
  
  ezUniquePtr();

  template <typename U>
  ezUniquePtr(const ezInternal::NewInstance<U>& instance);

  template <typename U>
  ezUniquePtr(U* pInstance, ezAllocatorBase* pAllocator);

  template <typename U>
  ezUniquePtr(ezUniquePtr<U>&& other);

  ~ezUniquePtr();

  template <typename U>
  void operator=(ezUniquePtr<U>&& other);

  T* Release();
  void Reset();

  T& operator*() const;
  T* operator->() const;
  operator bool() const;

  bool operator==(const ezUniquePtr<T>& rhs) const;
  bool operator!=(const ezUniquePtr<T>& rhs) const;
  bool operator<(const ezUniquePtr<T>& rhs) const;
  bool operator<=(const ezUniquePtr<T>& rhs) const;
  bool operator>(const ezUniquePtr<T>& rhs) const;
  bool operator>=(const ezUniquePtr<T>& rhs) const;

  bool operator==(nullptr_t) const;
  bool operator!=(nullptr_t) const;
  bool operator<(nullptr_t) const;
  bool operator<=(nullptr_t) const;
  bool operator>(nullptr_t) const;
  bool operator>=(nullptr_t) const;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezUniquePtr);

private:
  T* m_pInstance;
  ezAllocatorBase* m_pAllocator;
};

#include <Foundation/Types/Implementation/UniquePtr_inl.h>
