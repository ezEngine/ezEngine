
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

