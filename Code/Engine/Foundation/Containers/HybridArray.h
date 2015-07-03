#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation a dynamically growing array.
///
/// Best-case performance for the PushBack operation is in O(1) if the ezHybridArray does not need to be expanded.
/// In the worst case, PushBack is in O(n). 
/// Look-up is guaranteed to always be in O(1).
template <typename T, ezUInt32 Size>
class ezHybridArrayBase : public ezArrayBase<T, ezHybridArrayBase<T, Size> >
{
protected:
  /// \brief Creates an empty array. Does not allocate any data yet.
  ezHybridArrayBase(ezAllocatorBase* pAllocator); // [tested]
  
  /// \brief Creates a copy of the given array.
  ezHybridArrayBase(const ezHybridArrayBase<T, Size>& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the given array.
  ezHybridArrayBase(ezHybridArrayBase<T, Size>&& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezHybridArrayBase(const ezArrayPtr<const T>& other, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHybridArrayBase(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator= (const ezHybridArrayBase<T, Size>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator= (ezHybridArrayBase<T, Size>&& rhs); // [tested]

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
  T* GetStaticArray();

  /// \brief The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_StaticData[Size * sizeof(T)];
  };

  ezAllocatorBase* m_pAllocator;

  enum { CAPACITY_ALIGNMENT = 16 };

  void SetCapacity(ezUInt32 uiCapacity);
};


/// \brief \see ezHybridArrayBase
template <typename T, ezUInt32 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHybridArray : public ezHybridArrayBase<T, Size>
{
public:
  ezHybridArray();
  ezHybridArray(ezAllocatorBase* pAllocator);

  ezHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other);
  ezHybridArray(const ezHybridArrayBase<T, Size>& other);
  explicit ezHybridArray(const ezArrayPtr<const T>& other);

  ezHybridArray(ezHybridArray<T, Size, AllocatorWrapper>&& other);
  ezHybridArray(ezHybridArrayBase<T, Size>&& other);

  void operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const ezHybridArrayBase<T, Size>& rhs);
  void operator=(const ezArrayPtr<const T>& rhs);

  void operator=(ezHybridArray<T, Size, AllocatorWrapper>&& rhs);
  void operator=(ezHybridArrayBase<T, Size>&& rhs);

};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>

