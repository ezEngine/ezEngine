#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation a dynamically growing array.
///
/// Best-case performance for the PushBack operation is in O(1) if the ezDynamicArray does not need to be expanded.
/// In the worst case, PushBack is in O(n).
/// Look-up is guaranteed to always be in O(1).
template <typename T>
class ezDynamicArrayBase : public ezArrayBase<T, ezDynamicArrayBase<T> >
{
protected:
  /// \brief Creates an empty array. Does not allocate any data yet.
  ezDynamicArrayBase(ezAllocatorBase* pAllocator); // [tested]
  
  /// \brief Creates a copy of the given array.
  ezDynamicArrayBase(const ezDynamicArrayBase<T>& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the given array into this one.
  ezDynamicArrayBase(ezDynamicArrayBase<T>&& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezDynamicArrayBase(const ezArrayPtr<const T>& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezDynamicArrayBase(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator= (const ezDynamicArrayBase<T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator= (ezDynamicArrayBase<T>&& rhs); // [tested]

public:

  /// \brief Expands the array so it can at least store the given capacity.
  void Reserve(ezUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const { return m_pAllocator; }

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

private:
  ezAllocatorBase* m_pAllocator;

  enum { CAPACITY_ALIGNMENT = 16 };

  void SetCapacity(ezUInt32 uiCapacity);
};


/// \brief \see ezDynamicArrayBase
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezDynamicArray : public ezDynamicArrayBase<T>
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezDynamicArray();
  ezDynamicArray(ezAllocatorBase* pAllocator);

  ezDynamicArray(const ezDynamicArray<T, AllocatorWrapper>& other);
  ezDynamicArray(const ezDynamicArrayBase<T>& other);
  explicit ezDynamicArray(const ezArrayPtr<const T>& other);

  ezDynamicArray(ezDynamicArray<T, AllocatorWrapper>&& other);
  ezDynamicArray(ezDynamicArrayBase<T>&& other);

  void operator=(const ezDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const ezDynamicArrayBase<T>& rhs);
  void operator=(const ezArrayPtr<const T>& rhs);

  void operator=(ezDynamicArray<T, AllocatorWrapper>&& rhs);
  void operator=(ezDynamicArrayBase<T>&& rhs);
};

EZ_CHECK_AT_COMPILETIME_MSG(ezGetTypeClass<ezDynamicArray<int>>::value == 2, "dynamic array is not memory relocatable");

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>

