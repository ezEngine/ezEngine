
#pragma once

/// \brief A comparer object is used in sorting algorithms to compare to objects of the same type.
template <typename T>
struct ezCompareHelper
{
  /// \brief Returns true if a is less than b
  EZ_FORCE_INLINE static bool Less(const T& a, const T& b)
  {
    return a < b;
  }

  /// \brief Returns true if a is equal to b
  EZ_FORCE_INLINE static bool Equal(const T& a, const T& b)
  {
    return a == b;
  }
};

