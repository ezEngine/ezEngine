#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief Implementation a dynamically growing array.
///
/// Best-case performance for the PushBack operation is in O(1) if the ezHybridArray does not need to be expanded.
/// In the worst case, PushBack is in O(n).
/// Look-up is guaranteed to always be in O(1).
template <typename T, ezUInt32 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHybridArray : public ezDynamicArray<T, AllocatorWrapper>
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  /// \brief Creates an empty array. Does not allocate any data yet.
  ezHybridArray(); // [tested]

  /// \brief Creates an empty array. Does not allocate any data yet.
  ezHybridArray(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  ezHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other); // [tested]

  /// \brief Creates a copy of the given array.
  ezHybridArray(const ezArrayPtr<const T>& other); // [tested]

  /// \brief Moves the given array.
  ezHybridArray(ezHybridArray<T, Size, AllocatorWrapper>&& other); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezArrayPtr<const T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(ezHybridArray<T, Size, AllocatorWrapper>&& rhs); // [tested]

protected:

  /// \brief The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_StaticData[Size * sizeof(T)];
  };

  EZ_ALWAYS_INLINE T* GetStaticArray()
  {
    return reinterpret_cast<T*>(m_StaticData);
  }

  EZ_ALWAYS_INLINE const T* GetStaticArray() const
  {
    return reinterpret_cast<const T*>(m_StaticData);
  }
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>


