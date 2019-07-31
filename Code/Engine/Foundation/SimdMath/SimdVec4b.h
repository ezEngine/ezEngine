#pragma once

#include <Foundation/SimdMath/SimdTypes.h>
#include <Foundation/SimdMath/SimdSwizzle.h>

class EZ_FOUNDATION_DLL ezSimdVec4b
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4b();                               // [tested]
  ezSimdVec4b(bool b);                         // [tested]
  ezSimdVec4b(bool x, bool y, bool z, bool w); // [tested]
  ezSimdVec4b(ezInternal::QuadBool b);         // [tested]

public:
  template <int N>
  bool GetComponent() const; // [tested]

  bool x() const; // [tested]
  bool y() const; // [tested]
  bool z() const; // [tested]
  bool w() const; // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4b Get() const; // [tested]

public:
  ezSimdVec4b operator&&(const ezSimdVec4b& rhs) const; // [tested]
  ezSimdVec4b operator||(const ezSimdVec4b& rhs) const; // [tested]
  ezSimdVec4b operator!() const;                        // [tested]

  template <int N = 4>
  bool AllSet() const; // [tested]

  template <int N = 4>
  bool AnySet() const; // [tested]

  template <int N = 4>
  bool NoneSet() const; // [tested]

public:
  ezInternal::QuadBool m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

