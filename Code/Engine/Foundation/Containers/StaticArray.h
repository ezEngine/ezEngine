#pragma once

#include <Foundation/Containers/ArrayBase.h>

/// \brief Wraps a C-style array, which has a fixed size at compile-time, with a more convenient interface.
///
/// ezStaticArray can be used to create a fixed size array, either on the stack or as a class member.
/// Additionally it allows to use that array as a 'cache', i.e. not all its elements need to be constructed.
/// As such it can be used whenever a fixed size array is sufficient, but a more powerful interface is desired,
/// and when the number of elements in an array is dynamic at run-time, but always capped at a fixed limit.
template <typename T, ezUInt32 Capacity>
class ezStaticArray : public ezArrayBase<T, ezStaticArray<T, Capacity>>
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  /// \brief Creates an empty array.
  ezStaticArray(); // [tested]

  /// \brief Creates a copy of the given array.
  ezStaticArray(const ezStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  template <ezUInt32 OtherCapacity>
  ezStaticArray(const ezStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  explicit ezStaticArray(const ezArrayPtr<const T>& rhs); // [tested]

  /// \brief Destroys all objects.
  ~ezStaticArray(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  template <ezUInt32 OtherCapacity>
  void operator=(const ezStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const ezArrayPtr<const T>& rhs); // [tested]

  /// \brief For the static array Reserve is a no-op. However the function checks if the requested capacity is below or equal to the static capacity.
  void Reserve(ezUInt32 uiCapacity);

protected:
  T* GetElementsPtr();
  const T* GetElementsPtr() const;
  friend class ezArrayBase<T, ezStaticArray<T, Capacity>>;

private:
  T* GetStaticArray();
  const T* GetStaticArray() const;

  /// \brief The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_Data[Capacity * sizeof(T)];
  };

  friend class ezArrayBase<T, ezStaticArray<T, Capacity>>;
};

// TODO EZ_CHECK_AT_COMPILETIME_MSG with a ',' in the expression does not work
// EZ_CHECK_AT_COMPILETIME_MSG(ezGetTypeClass< ezStaticArray<int, 4> >::value == 2, "static array is not memory relocatable");

#include <Foundation/Containers/Implementation/StaticArray_inl.h>
