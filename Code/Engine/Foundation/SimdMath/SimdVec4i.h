#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

class ezSimdVec4u;

/// \brief A SIMD 4-component vector class of signed 32b integers
class EZ_FOUNDATION_DLL ezSimdVec4i
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4i();                                               // [tested]

  explicit ezSimdVec4i(ezInt32 iXyzw);                         // [tested]

  ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w = 1); // [tested]

  ezSimdVec4i(ezInternal::QuadInt v);                          // [tested]

  /// \brief Creates an ezSimdVec4i that is initialized to zero.
  [[nodiscard]] static ezSimdVec4i MakeZero();                     // [tested]

  void Set(ezInt32 iXyzw);                                         // [tested]

  void Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w);            // [tested]

  void SetZero();                                                  // [tested]

  template <int N>
  void Load(const ezInt32* pInts);                                 // [tested]

  template <int N>
  void Store(ezInt32* pInts) const;                                // [tested]

public:
  explicit ezSimdVec4i(const ezSimdVec4u& u);                      // [tested]

public:
  ezSimdVec4f ToFloat() const;                                     // [tested]

  [[nodiscard]] static ezSimdVec4i Truncate(const ezSimdVec4f& f); // [tested]

public:
  template <int N>
  ezInt32 GetComponent() const; // [tested]

  ezInt32 x() const;            // [tested]
  ezInt32 y() const;            // [tested]
  ezInt32 z() const;            // [tested]
  ezInt32 w() const;            // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4i Get() const;      // [tested]

  ///\brief x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <ezSwizzle::Enum s>
  [[nodiscard]] ezSimdVec4i GetCombined(const ezSimdVec4i& other) const;                                                 // [tested]

public:
  [[nodiscard]] ezSimdVec4i operator-() const;                                                                           // [tested]
  [[nodiscard]] ezSimdVec4i operator+(const ezSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] ezSimdVec4i operator-(const ezSimdVec4i& v) const;                                                       // [tested]

  [[nodiscard]] ezSimdVec4i CompMul(const ezSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] ezSimdVec4i CompDiv(const ezSimdVec4i& v) const;                                                         // [tested]

  [[nodiscard]] ezSimdVec4i operator|(const ezSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] ezSimdVec4i operator&(const ezSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] ezSimdVec4i operator^(const ezSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] ezSimdVec4i operator~() const;                                                                           // [tested]

  [[nodiscard]] ezSimdVec4i operator<<(ezUInt32 uiShift) const;                                                          // [tested]
  [[nodiscard]] ezSimdVec4i operator>>(ezUInt32 uiShift) const;                                                          // [tested]
  [[nodiscard]] ezSimdVec4i operator<<(const ezSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] ezSimdVec4i operator>>(const ezSimdVec4i& v) const;                                                      // [tested]

  ezSimdVec4i& operator+=(const ezSimdVec4i& v);                                                                         // [tested]
  ezSimdVec4i& operator-=(const ezSimdVec4i& v);                                                                         // [tested]

  ezSimdVec4i& operator|=(const ezSimdVec4i& v);                                                                         // [tested]
  ezSimdVec4i& operator&=(const ezSimdVec4i& v);                                                                         // [tested]
  ezSimdVec4i& operator^=(const ezSimdVec4i& v);                                                                         // [tested]

  ezSimdVec4i& operator<<=(ezUInt32 uiShift);                                                                            // [tested]
  ezSimdVec4i& operator>>=(ezUInt32 uiShift);                                                                            // [tested]

  [[nodiscard]] ezSimdVec4i CompMin(const ezSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] ezSimdVec4i CompMax(const ezSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] ezSimdVec4i Abs() const;                                                                                 // [tested]

  [[nodiscard]] ezSimdVec4b operator==(const ezSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] ezSimdVec4b operator!=(const ezSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] ezSimdVec4b operator<=(const ezSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] ezSimdVec4b operator<(const ezSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] ezSimdVec4b operator>=(const ezSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] ezSimdVec4b operator>(const ezSimdVec4i& v) const;                                                       // [tested]

  [[nodiscard]] static ezSimdVec4i Select(const ezSimdVec4b& vCmp, const ezSimdVec4i& vTrue, const ezSimdVec4i& vFalse); // [tested]

public:
  ezInternal::QuadInt m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4i_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4i_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4i_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
