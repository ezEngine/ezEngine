#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// A SIMD 4-component vector class of unsigned 32b integers
class EZ_FOUNDATION_DLL ezSimdVec4u
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4u();                                                   // [tested]

  explicit ezSimdVec4u(ezUInt32 uiXyzw);                           // [tested]

  ezSimdVec4u(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w = 1); // [tested]

  ezSimdVec4u(ezInternal::QuadUInt v);                             // [tested]

  /// Creates an ezSimdVec4u that is initialized to zero.
  [[nodiscard]] static ezSimdVec4u MakeZero();                     // [tested]

  void Set(ezUInt32 uiXyzw);                                       // [tested]

  void Set(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w);        // [tested]

  void SetZero();                                                  // [tested]

  template <int N>
  void Load(const ezUInt32* pUInts);                               // [tested]

  template <int N>
  void Store(ezUInt32* pUInts) const;                              // [tested]

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

  /// x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <ezSwizzle::Enum s>
  [[nodiscard]] ezSimdVec4u GetCombined(const ezSimdVec4u& other) const; // [tested]

public:
  [[nodiscard]] ezSimdVec4u operator+(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator-(const ezSimdVec4u& v) const; // [tested]

  [[nodiscard]] ezSimdVec4u CompMul(const ezSimdVec4u& v) const;   // [tested]
  [[nodiscard]] ezSimdVec4u CompDiv(const ezSimdVec4u& v) const;   // [tested]

  /// Subtracts eight packed unsigned 16-bit integers independently, clamping each result to zero.
  ///
  /// \code{.unparsed}
  /// for each component: [ highA ][  lowA ] - [ highB ][  lowB ]
  /// result:             [ max(highA - highB, 0) ][ max(lowA - lowB, 0) ]
  /// \endcode
  [[nodiscard]] ezSimdVec4u SubtractSaturated16(const ezSimdVec4u& v) const; // [tested]

  /// Transposes the 4x4 matrix of bytes stored in the four components.
  ///
  /// Each component is one row, with bytes ordered from least to most significant.
  ///
  /// \code{.unparsed}
  /// x [ a0 a1 a2 a3 ]    x [ a0 b0 c0 d0 ]
  /// y [ b0 b1 b2 b3 ] -> y [ a1 b1 c1 d1 ]
  /// z [ c0 c1 c2 c3 ] -> z [ a2 b2 c2 d2 ]
  /// w [ d0 d1 d2 d3 ]    w [ a3 b3 c3 d3 ]
  /// \endcode
  [[nodiscard]] ezSimdVec4u TransposeBytes() const;                // [tested]

  [[nodiscard]] ezSimdVec4u operator|(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator&(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator^(const ezSimdVec4u& v) const; // [tested]
  [[nodiscard]] ezSimdVec4u operator~() const;                     // [tested]

  [[nodiscard]] ezSimdVec4u operator<<(ezUInt32 uiShift) const;    // [tested]
  [[nodiscard]] ezSimdVec4u operator>>(ezUInt32 uiShift) const;    // [tested]

  /// Shifts each component left by the corresponding count. Counts >= 32 produce zero.
  [[nodiscard]] ezSimdVec4u operator<<(const ezSimdVec4u& vShift) const; // [tested]

  /// Shifts each component right logically by the corresponding count. Counts >= 32 produce zero.
  [[nodiscard]] ezSimdVec4u operator>>(const ezSimdVec4u& vShift) const; // [tested]

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

  [[nodiscard]] static ezSimdVec4u Select(const ezSimdVec4b& vCmp, const ezSimdVec4u& vTrue, const ezSimdVec4u& vFalse); // [tested]

  /// Reinterprets the bit pattern of this unsigned integer vector as a float vector. No conversion is performed.
  [[nodiscard]] ezSimdVec4f ReinterpretAsFloat() const;

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
