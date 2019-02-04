
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Hash helper to be used as a template argument to ezHashTable / ezHashSet for case insensitive string keys.
struct EZ_FOUNDATION_DLL ezHashHelperString_NoCase
{
  template <typename Derived>
  static ezUInt32 Hash(const ezStringBase<Derived>& value); // [tested]

  static ezUInt32 Hash(const char* szValue); // [tested]

  template <typename DerivedLhs, typename DerivedRhs>
  EZ_ALWAYS_INLINE static bool Equal(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs); // [tested]

  template <typename DerivedLhs>
  EZ_ALWAYS_INLINE static bool Equal(const ezStringBase<DerivedLhs>& lhs, const char* rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>

