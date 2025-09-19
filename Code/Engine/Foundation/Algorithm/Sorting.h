
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

/// \brief This class provides implementations of different sorting algorithms.
///
/// Algorithm selection guidelines:
/// - QuickSort: Fast general-purpose sorting. Best for large datasets (>16 elements). O(n log n) average, O(n²) worst case.
/// - InsertionSort: Efficient for small arrays or nearly sorted data. O(n²) worst case, O(n) best case.
///
/// The implementation automatically uses InsertionSort for subarrays smaller than 16 elements during QuickSort
/// to optimize performance for small partitions.
class ezSorting
{
public:
  /// \brief Sorts the elements in container using an in-place quicksort implementation (not stable).
  ///
  /// Performance: O(n log n) average case, O(n²) worst case, O(log n) space complexity.
  /// The worst case occurs with already sorted or reverse-sorted input, but this is rare in practice.
  /// Automatically switches to InsertionSort for small subarrays (<16 elements) for better performance.
  /// Not stable - equal elements may change their relative order.
  template <typename Container, typename Comparer>
  static void QuickSort(Container& inout_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using an in-place quicksort implementation (not stable).
  ///
  /// Performance: O(n log n) average case, O(n²) worst case, O(log n) space complexity.
  /// See the container overload for detailed performance characteristics.
  template <typename T, typename Comparer>
  static void QuickSort(ezArrayPtr<T>& inout_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using insertion sort (stable and in-place).
  ///
  /// Performance: O(n²) worst case, O(n) best case (nearly sorted), O(1) space complexity.
  /// Stable - equal elements maintain their relative order. More efficient than QuickSort for:
  /// - Small arrays (typically <16 elements)
  /// - Nearly sorted data (performs close to O(n))
  /// - When stability is required
  /// Less efficient for large unsorted datasets compared to QuickSort.
  template <typename Container, typename Comparer>
  static void InsertionSort(Container& inout_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using insertion sort (stable and in-place).
  ///
  /// Performance: O(n²) worst case, O(n) best case (nearly sorted), O(1) space complexity.
  /// See the container overload for detailed performance characteristics.
  template <typename T, typename Comparer>
  static void InsertionSort(ezArrayPtr<T>& inout_arrayPtr, const Comparer& comparer = Comparer()); // [tested]

private:
  enum
  {
    INSERTION_THRESHOLD = 16
  };

  // Perform comparison either with "Less(a,b)" (prefered) or with operator ()(a,b)
  template <typename Element, typename Comparer>
  EZ_ALWAYS_INLINE constexpr static auto DoCompare(const Comparer& comparer, const Element& a, const Element& b, int) -> decltype(comparer.Less(a, b))
  {
    return comparer.Less(a, b);
  }
  template <typename Element, typename Comparer>
  EZ_ALWAYS_INLINE constexpr static auto DoCompare(const Comparer& comparer, const Element& a, const Element& b, long) -> decltype(comparer(a, b))
  {
    return comparer(a, b);
  }
  template <typename Element, typename Comparer>
  EZ_ALWAYS_INLINE constexpr static bool DoCompare(const Comparer& comparer, const Element& a, const Element& b)
  {
    // Int/long is used to prefer the int version if both are available.
    // (Kudos to http://stackoverflow.com/a/9154394/5347927 where I've learned this trick)
    return DoCompare(comparer, a, b, 0);
  }


  template <typename Container, typename Comparer>
  static void QuickSort(Container& inout_container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename Container, typename Comparer>
  static ezUInt32 Partition(Container& inout_container, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer);


  template <typename T, typename Comparer>
  static void QuickSort(ezArrayPtr<T>& inout_arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static ezUInt32 Partition(T* pPtr, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void InsertionSort(ezArrayPtr<T>& inout_arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer);
};

#include <Foundation/Algorithm/Implementation/Sorting_inl.h>
