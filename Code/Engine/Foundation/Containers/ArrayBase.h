#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Basics/Types/ArrayPtr.h>

#define ezInvalidIndex 0xFFFFFFFF

/// Base class for all array containers. Implements all the basic functionality that only requires a pointer and the element count.
template <typename T, typename Derived>
class ezArrayBase
{
public:
  ezArrayBase();
  
  ~ezArrayBase();

  /// Conversion to const ezArrayPtr.
  operator const ezArrayPtr<T>() const; // [tested]

  /// Conversion to ezArrayPtr.
  operator ezArrayPtr<T>(); // [tested]

  /// Compares this array to another contiguous array type.
  bool operator== (const ezArrayPtr<T>& rhs) const; // [tested]

  /// Compares this array to another contiguous array type.
  bool operator!= (const ezArrayPtr<T>& rhs) const; // [tested]
  
  /// Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]

  /// Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// Returns the number of active elements in the array.
  ezUInt32 GetCount() const; // [tested]

  /// Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the array.
  void Clear(); // [tested]

  /// Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// Appends value at the end of the array. Increases the capacity if necessary. Returns the index of the added element.
  ezUInt32 Append(const T& value); // [tested]

  /// Appends value at the end of the array. Does NOT ensure capacity. Returns the index of the added element.
  ezUInt32 AppendUnchecked(const T& value); // [tested]

  /// Appends all elements in range at the end of the array. Increases the capacity if necessary.
  void AppendRange(const ezArrayPtr<T>& range); // [tested]

  /// Appends all elements in range at the end of the array. Increases the capacity if necessary.
  void AppendRange(const ezArrayPtr<const T>& range); // [tested]

  /// Inserts value at index by shifting all following elements.
  void Insert(const T& value, ezUInt32 uiIndex); // [tested]

  /// Removes the first occurrence of value and fills the gap by shifting all following elements
  bool Remove(const T& value); // [tested]

  /// Removes the element at index and fills the gap by shifting all following elements
  void RemoveAt(ezUInt32 uiIndex); // [tested]

  /// Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtSwap(ezUInt32 uiIndex); // [tested]

  /// Searches for the first occurrence of the given value and returns its index or InvalidIndex if not found.
  ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const; // [tested]

  /// Searches for the last occurrence of the given value and returns its index or InvalidIndex if not found. 
  ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezInvalidIndex) const; // [tested]

  /// Pushes value at the end of the array.
  void Push(const T& value); // [tested]

  /// Removes count elements from the end of the array.
  void Pop(ezUInt32 uiCountToRemove = 1); // [tested]

  /// Returns the last element of the arry.
  T& Peek(); // [tested]

  /// Returns the last element of the arry.
  const T& Peek() const; // [tested]

  /// Sort with explicit comparer
  template <typename C>
  void Sort();

  /// Sort with default comparer
  void Sort();

protected:

  /// Element-type access to m_Data.
  T* m_pElements;

  /// The number of elements used from the array.
  ezUInt32 m_uiCount;

  /// The number of elements which can be stored in the array without re-allocating.
  ezUInt32 m_uiCapacity;
};

#include <Foundation/Containers/Implementation/ArrayBase_inl.h>
