#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// \brief A SIMD 4-component vector class of signed 32b integers
class EZ_FOUNDATION_DLL ezSimdVec4i
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4i(); // [tested]

  explicit ezSimdVec4i(ezInt32 xyzw); // [tested]

  ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w = 1); // [tested]

  ezSimdVec4i(ezInternal::QuadInt v); // [tested]

  void Set(ezInt32 xyzw); // [tested]

  void Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w); // [tested]

  void SetZero(); // [tested]

public:
  ezSimdVec4f ToFloat() const; // [tested]

  static ezSimdVec4i Truncate(const ezSimdVec4f& f); // [tested]

public:
  template <int N>
  ezInt32 GetComponent() const; // [tested]

  ezInt32 x() const; // [tested]
  ezInt32 y() const; // [tested]
  ezInt32 z() const; // [tested]
  ezInt32 w() const; // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4i Get() const; // [tested]

public:
  ezSimdVec4i operator-() const;                     // [tested]
  ezSimdVec4i operator+(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i operator-(const ezSimdVec4i& v) const; // [tested]

  ezSimdVec4i CompMul(const ezSimdVec4i& v) const; // [tested]

  ezSimdVec4i operator|(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i operator&(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i operator^(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i operator~() const;                     // [tested]

  ezSimdVec4i operator<<(ezUInt32 uiShift) const; // [tested]
  ezSimdVec4i operator>>(ezUInt32 uiShift) const; // [tested]

  ezSimdVec4i& operator+=(const ezSimdVec4i& v); // [tested]
  ezSimdVec4i& operator-=(const ezSimdVec4i& v); // [tested]

  ezSimdVec4i& operator|=(const ezSimdVec4i& v); // [tested]
  ezSimdVec4i& operator&=(const ezSimdVec4i& v); // [tested]
  ezSimdVec4i& operator^=(const ezSimdVec4i& v); // [tested]

  ezSimdVec4i& operator<<=(ezUInt32 uiShift); // [tested]
  ezSimdVec4i& operator>>=(ezUInt32 uiShift); // [tested]

  ezSimdVec4i CompMin(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i CompMax(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4i Abs() const;                         // [tested]

  ezSimdVec4b operator==(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4b operator!=(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4b operator<=(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4b operator<(const ezSimdVec4i& v) const;  // [tested]
  ezSimdVec4b operator>=(const ezSimdVec4i& v) const; // [tested]
  ezSimdVec4b operator>(const ezSimdVec4i& v) const;  // [tested]

  static ezSimdVec4i ZeroVector(); // [tested]

public:
  ezInternal::QuadInt m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4i_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4i_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
