#pragma once

#include <Foundation/Containers/ArrayBase.h>

template <typename T, ezUInt32 Capacity>
class ezStaticArray : public ezArrayBase<T, ezStaticArray<T, Capacity> >
{
public:
  /// \brief Creates an empty array.
  ezStaticArray(); // [tested]

  /// \brief Creates a copy of the given array.
  ezStaticArray(const ezStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  template<ezUInt32 OtherCapacity>
  ezStaticArray(const ezStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  explicit ezStaticArray(const ezArrayPtr<T>& rhs); // [tested]

  /// \brief Destroys all objects.
  ~ezStaticArray(); // [tested]
  
  /// \brief Copies the data from some other contiguous array into this one.
  void operator= (const ezStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  template<ezUInt32 OtherCapacity>
  void operator= (const ezStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator= (const ezArrayPtr<T>& rhs); // [tested]
  
  /// \brief Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt32 uiCount); // [tested]

private:
  T* GetStaticArray();

  /// \brief The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_Data[Capacity * sizeof(T)];
  };

  friend class ezArrayBase<T, ezStaticArray<T, Capacity> >;
  void Reserve(ezUInt32 uiCapacity);
};

#include <Foundation/Containers/Implementation/StaticArray_inl.h>
