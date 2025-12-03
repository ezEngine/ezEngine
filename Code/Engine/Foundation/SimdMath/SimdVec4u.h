#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// \brief A SIMD 4-component vector class of unsigned 32b integers
class EZ_FOUNDATION_DLL ezSimdVec4u
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4u();                                                   // [tested]

  explicit ezSimdVec4u(ezUInt32 uiXyzw);                           // [tested]

  ezSimdVec4u(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w = 1); // [tested]

  ezSimdVec4u(ezInternal::QuadUInt v);                             // [tested]

  /// \brief Creates an ezSimdVec4u that is initialized to zero.
  [[nodiscard]] static ezSimdVec4u MakeZero();                     // [tested]

  void Set(ezUInt32 uiXyzw);                                       // [tested]

  void Set(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w);        // [tested]

  void SetZero();                                                  // [tested]

public:
  explicit ezSimdVec4u(const ezSimdVec4i& i);                      // [tested]

public:
  ezSimdVec4f ToFloat() const;                                     // [tested]

  [[nodiscard]] static ezSimdVec4u Truncate(const ezSimdVec4f& f); // [tested]

public:
  template <int N>
  ezUInt32 GetComponent() const;                                   // [tested]

  ezUInt32 x() const;                                              // [tested]
  ezUInt32 y() const;                                              // [tested]
  ezUInt32 z() const;                                              // [tested]
  ezUInt32 w() const;                                              // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4u Get() const;                                         // [tested]

public:
  [[nodiscard]] ezSimdVec4u operator+(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator-(const ezSimdVec4u& v) const; // [tested]

  [[nodiscard]] ezSimdVec4u CompMul(const ezSimdVec4u& v) const;   // [tested]

  [[nodiscard]] ezSimdVec4u operator|(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator&(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator^(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator~() const;                     // [tested]

  [[nodiscard]] ezSimdVec4u operator<<(ezUInt32 uiShift) const;    // [tested]
  [[nodiscard]] ezSimdVec4u operator>>(ezUInt32 uiShift) const;    // [tested]

  ezSimdVec4u& operator+=(const ezSimdVec4u& v);                   // [tested]
  ezSimdVec4u& operator-=(const ezSimdVec4u& v);                   // [tested]

  ezSimdVec4u& operator|=(const ezSimdVec4u& v);                   // [tested]
  ezSimdVec4u& operator&=(const ezSimdVec4u& v);                   // [tested]
  ezSimdVec4u& operator^=(const ezSimdVec4u& v);                   // [tested]

  ezSimdVec4u& operator<<=(ezUInt32 uiShift);                      // [tested]
  ezSimdVec4u& operator>>=(ezUInt32 uiShift);                      // [tested]

  [[nodiscard]] ezSimdVec4u CompMin(const ezSimdVec4u& v) const;   // [tested]
  [[nodiscard]] ezSimdVec4u CompMax(const ezSimdVec4u& v) const;   // [tested]

  ezSimdVec4b operator==(const ezSimdVec4u& v) const;              // [tested]
  ezSimdVec4b operator!=(const ezSimdVec4u& v) const;              // [tested]
  ezSimdVec4b operator<=(const ezSimdVec4u& v) const;              // [tested]
  ezSimdVec4b operator<(const ezSimdVec4u& v) const;               // [tested]
  ezSimdVec4b operator>=(const ezSimdVec4u& v) const;              // [tested]
  ezSimdVec4b operator>(const ezSimdVec4u& v) const;               // [tested]

public:
  ezInternal::QuadUInt m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4u_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4u_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4u_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
