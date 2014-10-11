#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

/// \brief Value used by containers for indices to indicate an invalid index.
#define ezInvalidIndex 0xFFFFFFFF

/// \brief Base class for all array containers. Implements all the basic functionality that only requires a pointer and the element count.
template <typename T, typename Derived>
class ezArrayBase
{
public:
  /// \brief Constructor.
  ezArrayBase(); // [tested]
  
  /// \brief Destructor.
  ~ezArrayBase(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator= (const ezArrayPtr<T>& rhs); // [tested]

  /// \brief Conversion to const ezArrayPtr.
  operator const ezArrayPtr<T>() const; // [tested]

  /// \brief Conversion to ezArrayPtr.
  operator ezArrayPtr<T>(); // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator== (const ezArrayPtr<T>& rhs) const; // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator!= (const ezArrayPtr<T>& rhs) const; // [tested]
  
  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt32 uiCount); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  void SetCountUninitialized(ezUInt32 uiCount); // [tested]

  /// \brief Returns the number of active elements in the array.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the array.
  void Clear(); // [tested]

  /// \brief Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(const T& value, ezUInt32 uiIndex); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by shifting all following elements
  bool Remove(const T& value); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveSwap(const T& value); // [tested]

  /// \brief Removes the element at index and fills the gap by shifting all following elements
  void RemoveAt(ezUInt32 uiIndex); // [tested]

  /// \brief Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtSwap(ezUInt32 uiIndex); // [tested]

  /// \brief Searches for the first occurrence of the given value and returns its index or ezInvalidIndex if not found.
  ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const; // [tested]

  /// \brief Searches for the last occurrence of the given value and returns its index or ezInvalidIndex if not found. 
  ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezInvalidIndex) const; // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(const T& value); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// \brief Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const ezArrayPtr<typename ezTypeTraits<T>::NonConstType>& range); // [tested]

  /// \brief Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const ezArrayPtr<const T>& range); // [tested]

  /// \brief Removes count elements from the end of the array.
  void PopBack(ezUInt32 uiCountToRemove = 1); // [tested]

  /// \brief Returns the last element of the array.
  T& PeekBack(); // [tested]

  /// \brief Returns the last element of the array.
  const T& PeekBack() const; // [tested]

  /// \brief Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Sort with default comparer
  void Sort(); // [tested]

protected:

  /// \brief Element-type access to m_Data.
  T* m_pElements;

  /// \brief The number of elements used from the array.
  ezUInt32 m_uiCount;

  /// \brief The number of elements which can be stored in the array without re-allocating.
  ezUInt32 m_uiCapacity;
};

#include <Foundation/Containers/Implementation/ArrayBase_inl.h>

