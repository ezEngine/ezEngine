#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/PointerWithFlags.h>

/// \brief Implementation of a dynamically growing array.
///
/// Best-case performance for the PushBack operation is O(1) if the ezDynamicArray doesn't need to be expanded.
/// In the worst case, PushBack is O(n).
/// Look-up is guaranteed to always be O(1).
template <typename T>
class ezDynamicArrayBase : public ezArrayBase<T, ezDynamicArrayBase<T>>
{
protected:
  /// \brief Creates an empty array. Does not allocate any data yet.
  explicit ezDynamicArrayBase(ezAllocator* pAllocator);                                 // [tested]

  ezDynamicArrayBase(T* pInplaceStorage, ezUInt32 uiCapacity, ezAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezDynamicArrayBase(const ezDynamicArrayBase<T>& other, ezAllocator* pAllocator); // [tested]

  /// \brief Moves the given array into this one.
  ezDynamicArrayBase(ezDynamicArrayBase<T>&& other, ezAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezDynamicArrayBase(const ezArrayPtr<const T>& other, ezAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezDynamicArrayBase(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezDynamicArrayBase<T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(ezDynamicArrayBase<T>&& rhs) noexcept; // [tested]

  T* GetElementsPtr();
  const T* GetElementsPtr() const;

  friend class ezArrayBase<T, ezDynamicArrayBase<T>>;

public:
  /// \brief Expands the array so it can at least store the given capacity.
  void Reserve(ezUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const { return const_cast<ezAllocator*>(m_pAllocator.GetPtr()); }

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief swaps the contents of this array with another one
  void Swap(ezDynamicArrayBase<T>& other); // [tested]

private:
  enum Storage
  {
    Owned = 0,
    External = 1
  };

  ezPointerWithFlags<ezAllocator, 1> m_pAllocator;

  enum
  {
    CAPACITY_ALIGNMENT = 16
  };

  void SetCapacity(ezUInt32 uiCapacity);
};

/// \brief \see ezDynamicArrayBase
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezDynamicArray : public ezDynamicArrayBase<T>
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();


  ezDynamicArray();
  explicit ezDynamicArray(ezAllocator* pAllocator);

  ezDynamicArray(const ezDynamicArray<T, AllocatorWrapper>& other);
  ezDynamicArray(const ezDynamicArrayBase<T>& other);
  explicit ezDynamicArray(const ezArrayPtr<const T>& other);

  ezDynamicArray(ezDynamicArray<T, AllocatorWrapper>&& other);
  ezDynamicArray(ezDynamicArrayBase<T>&& other);

  void operator=(const ezDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const ezDynamicArrayBase<T>& rhs);
  void operator=(const ezArrayPtr<const T>& rhs);

  void operator=(ezDynamicArray<T, AllocatorWrapper>&& rhs) noexcept;
  void operator=(ezDynamicArrayBase<T>&& rhs) noexcept;

protected:
  ezDynamicArray(T* pInplaceStorage, ezUInt32 uiCapacity, ezAllocator* pAllocator)
    : ezDynamicArrayBase<T>(pInplaceStorage, uiCapacity, pAllocator)
  {
  }
};

/// Overload of ezMakeArrayPtr for const dynamic arrays of pointer pointing to const type.
template <typename T, typename AllocatorWrapper>
ezArrayPtr<const T* const> ezMakeArrayPtr(const ezDynamicArray<T*, AllocatorWrapper>& dynArray);

/// Overload of ezMakeArrayPtr for const dynamic arrays.
template <typename T, typename AllocatorWrapper>
ezArrayPtr<const T> ezMakeArrayPtr(const ezDynamicArray<T, AllocatorWrapper>& dynArray);

/// Overload of ezMakeArrayPtr for dynamic arrays.
template <typename T, typename AllocatorWrapper>
ezArrayPtr<T> ezMakeArrayPtr(ezDynamicArray<T, AllocatorWrapper>& in_dynArray);


static_assert(ezGetTypeClass<ezDynamicArray<int>>::value == 2, "dynamic array is not memory relocatable");

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>
