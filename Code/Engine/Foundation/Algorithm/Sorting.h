
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Math/Math.h>

/// \brief This class provides implementations of different sorting algorithms.
template <typename Comparer>
class ezSorting
{
public:
  /// \brief Sorts the elements in container using a in-place quick sort implementation (not stable).
  template <typename Container>
  static void QuickSort(Container& container); // [tested]

  /// \brief Sorts the elements in the array using a in-place quick sort implementation (not stable).
  template <typename T>
  static void QuickSort(ezArrayPtr<T>& arrayPtr); // [tested]


  /// \brief Sorts the elements in container using insertion sort (stable and in-place).
  template <typename Container>
  static void InsertionSort(Container& container); // [tested]

  /// \brief Sorts the elements in the array using insertion sort (stable and in-place).
  template <typename T>
  static void InsertionSort(ezArrayPtr<T>& arrayPtr); // [tested]

private:
  enum { INSERTION_THRESHOLD = 16 };

  template <typename Container>
  static void QuickSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex);

  template <typename Container>
  static ezUInt32 Partition(Container& container, ezUInt32 uiLeft, ezUInt32 uiRight);


  template <typename T>
  static void QuickSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex);

  template <typename T>
  static ezUInt32 Partition(ezArrayPtr<T>& arrayPtr, ezUInt32 uiLeft, ezUInt32 uiRight);


  template <typename Container>
  static void InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex);

  template <typename T>
  static void InsertionSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex);
};

#include <Foundation/Algorithm/Implementation/Sorting_inl.h>

