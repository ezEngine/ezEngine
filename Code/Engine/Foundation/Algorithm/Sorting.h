
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Math/Math.h>

template <typename Comparer>
class ezSorting
{
public:
  /// Sorts the elements in container using a in-place quick sort implementation (not stable)
  template <typename Container>
  static void QuickSort(Container& container); 

  /// Sorts the elements in the array using a in-place quick sort implementation (not stable)
  template <typename T>
  static void QuickSort(ezArrayPtr<T>& arrayPtr);


  /// Sorts the elements in container using insertion sort (stable and in-place)
  template <typename Container>
  static void InsertionSort(Container& container);

  /// Sorts the elements in the array using insertion sort (stable and in-place)
  template <typename T>
  static void InsertionSort(ezArrayPtr<T>& arrayPtr);

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

