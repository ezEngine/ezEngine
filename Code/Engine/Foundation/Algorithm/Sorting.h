
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Math/Math.h>

/// \brief This class provides implementations of different sorting algorithms.
class ezSorting
{
public:
  /// \brief Sorts the elements in container using a in-place quick sort implementation (not stable).
  template <typename Container, typename Comparer>
  static void QuickSort(Container& container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using a in-place quick sort implementation (not stable).
  template <typename T, typename Comparer>
  static void QuickSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using insertion sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void InsertionSort(Container& container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using insertion sort (stable and in-place).
  template <typename T, typename Comparer>
  static void InsertionSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer = Comparer()); // [tested]

private:
  enum { INSERTION_THRESHOLD = 16 };

  template <typename Container, typename Comparer>
  static void QuickSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename Container, typename Comparer>
  static ezUInt32 Partition(Container& container, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer);


  template <typename T, typename Comparer>
  static void QuickSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static ezUInt32 Partition(ezArrayPtr<T>& arrayPtr, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void InsertionSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);
};

#include <Foundation/Algorithm/Implementation/Sorting_inl.h>

