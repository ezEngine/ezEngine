#pragma once

#include <Foundation/Containers/ArrayBase.h>

/// Base class for a dynamically growing array.
template <typename T>
class ezDynamicArrayBase : public ezArrayBase<T, ezDynamicArrayBase<T> >
{
protected:
  /// Creates an empty array. Does not allocate any data yet.
  ezDynamicArrayBase(ezIAllocator* pAllocator); // [tested]
  
  /// Creates a copy of the given array.
  ezDynamicArrayBase(const ezDynamicArrayBase<T>& other, ezIAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  ezDynamicArrayBase(const ezArrayPtr<T>& other, ezIAllocator* pAllocator); // [tested]

  ~ezDynamicArrayBase(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator= (const ezDynamicArrayBase<T>& rhs); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator= (const ezArrayPtr<T>& rhs); // [tested]

public:
  /// Expands the array so it can at least store the given capacity.
  void Reserve(ezUInt32 uiCapacity);

  /// Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will deallocate all data, if the array is empty.
  void Compact();

  /// Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt32 uiCount); // [tested]

  /// Returns the allocator that is used by this instance.
  ezIAllocator* GetAllocator() const { return m_pAllocator; }

private:
  ezIAllocator* m_pAllocator;

  enum { CAPACITY_ALIGNMENT = 16 };

  void SetCapacity(ezUInt32 uiCapacity);
};

/// Implementation a dynamically growing array. Best-case performance for the Append operation is
/// in O(1) if the ezDynamicArray does not need to be expanded. In the worst case, Append is in O(n). 
/// Look-up is guaranteed to always be in O(1).
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezDynamicArray : public ezDynamicArrayBase<T>
{
public:
  ezDynamicArray();
  ezDynamicArray(ezIAllocator* pAllocator);

  ezDynamicArray(const ezDynamicArray<T, AllocatorWrapper>& other);
  ezDynamicArray(const ezDynamicArrayBase<T>& other);
  ezDynamicArray(const ezArrayPtr<T>& other);

  void operator=(const ezDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const ezDynamicArrayBase<T>& rhs);
  void operator=(const ezArrayPtr<T>& rhs);
};

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>
