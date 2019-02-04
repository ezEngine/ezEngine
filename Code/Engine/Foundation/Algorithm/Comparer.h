
#pragma once

/// \brief A comparer object is used in sorting algorithms to compare to objects of the same type.
template <typename T>
struct ezCompareHelper
{
  /// \brief Returns true if a is less than b
  EZ_ALWAYS_INLINE bool Less(const T& a, const T& b) const
  {
    return a < b;
  }

  /// \brief Returns true if a is equal to b
  EZ_ALWAYS_INLINE bool Equal(const T& a, const T& b) const
  {
    return a == b;
  }
};

// See <Foundation/Strings/String.h> for ezString specialization and case insensitive version.

