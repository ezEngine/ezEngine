#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef ezInvalidIndex
#  define ezInvalidIndex 0xFFFFFFFF
#endif

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
  void operator=(const ezArrayPtr<const T>& rhs); // [tested]

  /// \brief Conversion to const ezArrayPtr.
  operator ezArrayPtr<const T>() const; // [tested]

  /// \brief Conversion to ezArrayPtr.
  operator ezArrayPtr<T>(); // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator==(const ezArrayPtr<const T>& rhs) const; // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator!=(const ezArrayPtr<const T>& rhs) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt32 uiCount); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Constructs all new elements by copying the FillValue.
  void SetCount(ezUInt32 uiCount, const T& FillValue); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  template <typename = void> // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(ezUInt32 uiCount); // [tested]

  /// \brief Ensures the container has at least \a uiCount elements. Ie. calls SetCount() if the container has fewer elements, does nothing
  /// otherwise.
  void EnsureCount(ezUInt32 uiCount); // [tested]

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

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(T&& value, ezUInt32 uiIndex); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by shifting all following elements
  bool RemoveAndCopy(const T& value); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveAndSwap(const T& value); // [tested]

  /// \brief Removes the element at index and fills the gap by shifting all following elements
  void RemoveAtAndCopy(ezUInt32 uiIndex, ezUInt32 uiNumElements = 1); // [tested]

  /// \brief Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtAndSwap(ezUInt32 uiIndex, ezUInt32 uiNumElements = 1); // [tested]

  /// \brief Searches for the first occurrence of the given value and returns its index or ezInvalidIndex if not found.
  ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const; // [tested]

  /// \brief Searches for the last occurrence of the given value and returns its index or ezInvalidIndex if not found.
  ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezInvalidIndex) const; // [tested]

  /// \brief Grows the array by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(const T& value); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(T&& value); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(T&& value); // [tested]

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

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  T* GetData();

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  const T* GetData() const;

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<T> GetArrayPtr(); // [tested]

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<const T> GetArrayPtr() const; // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<typename ezArrayPtr<T>::ByteType> GetByteArrayPtr(); // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<typename ezArrayPtr<const T>::ByteType> GetByteArrayPtr() const; // [tested]

  /// \brief Returns the reserved number of elements that the array can hold without reallocating.
  ezUInt32 GetCapacity() const { return m_uiCapacity; }

  using const_iterator = const T *;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T *;
  using reverse_iterator = reverse_pointer_iterator<T>;

protected:
  void DoSwap(ezArrayBase<T, Derived>& other);

  /// \brief Element-type access to m_Data.
  T* m_pElements = nullptr;

  /// \brief The number of elements used from the array.
  ezUInt32 m_uiCount = 0;

  /// \brief The number of elements which can be stored in the array without re-allocating.
  ezUInt32 m_uiCapacity = 0;
};

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::iterator begin(ezArrayBase<T, Derived>& container)
{
  return container.GetData();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_iterator begin(const ezArrayBase<T, Derived>& container)
{
  return container.GetData();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_iterator cbegin(const ezArrayBase<T, Derived>& container)
{
  return container.GetData();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::reverse_iterator rbegin(ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_reverse_iterator rbegin(const ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_reverse_iterator crbegin(const ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::iterator end(ezArrayBase<T, Derived>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_iterator end(const ezArrayBase<T, Derived>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_iterator cend(const ezArrayBase<T, Derived>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::reverse_iterator rend(ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::reverse_iterator(container.GetData() - 1);
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_reverse_iterator rend(const ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, typename Derived>
typename ezArrayBase<T, Derived>::const_reverse_iterator crend(const ezArrayBase<T, Derived>& container)
{
  return typename ezArrayBase<T, Derived>::const_reverse_iterator(container.GetData() - 1);
}


#include <Foundation/Containers/Implementation/ArrayBase_inl.h>
