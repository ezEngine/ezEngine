
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Hash helper to be used as a template argument to ezHashTable / ezHashSet for case insensitive string keys.
struct EZ_FOUNDATION_DLL ezHashHelperString_NoCase
{
  inline static ezUInt32 Hash(ezStringView sValue);                       // [tested]

  EZ_ALWAYS_INLINE static bool Equal(ezStringView lhs, ezStringView rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>
