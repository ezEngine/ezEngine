#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief A hybrid array uses in-place storage to handle the first few elements without any allocation. It dynamically resizes when more elements are needed.
///
/// It is often more efficient to use a hybrid array, rather than a dynamic array, when the number of needed elements is typically low or when the array is used only temporarily. In this case costly allocations can often be prevented entirely.
/// However, if the number of elements is unpredictable or usually very large, prefer a dynamic array, to avoid wasting (stack) memory for a hybrid array that is rarely large enough to be used.
/// The ezHybridArray is derived from ezDynamicArray and can therefore be passed to functions that expect an ezDynamicArray, even for output.
template <typename T, ezUInt32 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHybridArray : public ezDynamicArray<T, AllocatorWrapper>
{
public:
  /// \brief Creates an empty array. Does not allocate any data yet.
  ezHybridArray(); // [tested]

  /// \brief Creates an empty array. Does not allocate any data yet.
  explicit ezHybridArray(ezAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other); // [tested]

  /// \brief Creates a copy of the given array.
  explicit ezHybridArray(const ezArrayPtr<const T>& other); // [tested]

  /// \brief Moves the given array.
  ezHybridArray(ezHybridArray<T, Size, AllocatorWrapper>&& other) noexcept; // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezArrayPtr<const T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(ezHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept; // [tested]

protected:
  /// \brief The fixed size array.
  struct alignas(alignof(T))
  {
    ezUInt8 m_StaticData[Size * sizeof(T)];
  };

  EZ_ALWAYS_INLINE T* GetStaticArray() { return reinterpret_cast<T*>(m_StaticData); }

  EZ_ALWAYS_INLINE const T* GetStaticArray() const { return reinterpret_cast<const T*>(m_StaticData); }
};

/// A hybrid array that uses the temp allocator if it exceeds the in-place storage.
/// This is ideal for temporary arrays that are only used within a short scope and are not expected to grow beyond the in-place storage size in most cases.
/// The temp allocator is optimized for short-lived allocations and can be more efficient than the default allocator for this use case.
template <typename T, ezUInt32 Size>
class ezTempHybridArray : public ezHybridArray<T, Size>
{
public:
  ezTempHybridArray();

  template <typename AllocatorWrapper>
  ezTempHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other);
  explicit ezTempHybridArray(const ezArrayPtr<const T>& other);

  template <typename AllocatorWrapper>
  void operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const ezArrayPtr<const T>& rhs);

  void operator=(ezHybridArray<T, Size>&& rhs) noexcept;
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>
