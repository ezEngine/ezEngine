#pragma once

#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdTypes.h>


class EZ_FOUNDATION_DLL ezSimdVec4bWide
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4bWide();                               // [tested]
  ezSimdVec4bWide(bool b);                         // [tested]
  ezSimdVec4bWide(bool x, bool y, bool z, bool w); // [tested]
  ezSimdVec4bWide(ezInternal::QuadBoolWide b);         // [tested]

public:
  template <int N>
  bool GetComponent() const;                                                                               // [tested]

  bool x() const;                                                                                          // [tested]
  bool y() const;                                                                                          // [tested]
  bool z() const;                                                                                          // [tested]
  bool w() const;                                                                                          // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4bWide Get() const;                                                                                 // [tested]

public:
  ezSimdVec4bWide operator&&(const ezSimdVec4bWide& rhs) const;                                                    // [tested]
  ezSimdVec4bWide operator||(const ezSimdVec4bWide& rhs) const;                                                    // [tested]
  ezSimdVec4bWide operator!() const;                                                                           // [tested]

  ezSimdVec4bWide operator==(const ezSimdVec4bWide& rhs) const;                                                    // [tested]
  ezSimdVec4bWide operator!=(const ezSimdVec4bWide& rhs) const;                                                    // [tested]

  template <int N = 4>
  bool AllSet() const;                                                                                     // [tested]

  template <int N = 4>
  bool AnySet() const;                                                                                     // [tested]

  template <int N = 4>
  bool NoneSet() const;                                                                                    // [tested]

  static ezSimdVec4bWide Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4bWide& vTrue, const ezSimdVec4bWide& vFalse); // [tested]

public:
  ezInternal::QuadBoolWide m_v;
};









#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_Wide_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_Wide_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4b_Wide_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
