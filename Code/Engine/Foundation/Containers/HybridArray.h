#pragma once

#include <Foundation/Containers/ArrayBase.h>

/// Implementation a dynamically growing array. Best-case performance for the Append operation is
/// in O(1) if the ezHybridArray does not need to be expanded. In the worst case, Append is in O(n). 
/// Look-up is guaranteed to always be in O(1).
template <typename T, ezUInt32 Size>
class ezHybridArrayBase : public ezArrayBase<T, ezHybridArrayBase<T, Size> >
{
protected:
  /// Creates an empty array. Does not allocate any data yet.
  ezHybridArrayBase(ezIAllocator* pAllocator); // [tested]
  
  /// Creates a copy of the given array.
  ezHybridArrayBase(const ezHybridArrayBase<T, Size>& other, ezIAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  ezHybridArrayBase(const ezArrayPtr<T>& other, ezIAllocator* pAllocator); // [tested]

  ~ezHybridArrayBase(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator= (const ezHybridArrayBase<T, Size>& rhs); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator= (const ezArrayPtr<T>& rhs); // [tested]

public:
  /// Expands the array so it can at least store the given capacity.
  void Reserve(ezUInt32 uiCapacity);

  /// Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt32 uiCount); // [tested]

  /// Returns the allocator that is used by this instance.
  ezIAllocator* GetAllocator() const { return m_pAllocator; }

private:
  T* GetStaticArray();

  /// The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_StaticData[Size * sizeof(T)];
  };

  ezIAllocator* m_pAllocator;

  enum { CAPACITY_ALIGNMENT = 16 };

  void SetCapacity(ezUInt32 uiCapacity);
};

template <typename T, ezUInt32 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHybridArray : public ezHybridArrayBase<T, Size>
{
public:
  ezHybridArray();
  ezHybridArray(ezIAllocator* pAllocator);

  ezHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other);
  ezHybridArray(const ezHybridArrayBase<T, Size>& other);
  ezHybridArray(const ezArrayPtr<T>& other);

  void operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const ezHybridArrayBase<T, Size>& rhs);
  void operator=(const ezArrayPtr<T>& rhs);
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>
