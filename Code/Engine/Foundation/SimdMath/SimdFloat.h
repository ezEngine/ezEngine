#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdMath.h>

class EZ_FOUNDATION_DLL ezSimdFloat
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor, leaves the data uninitialized.
  ezSimdFloat(); // [tested]

  /// \brief Constructs from a given float.
  ezSimdFloat(float f); // [tested]

  /// \brief Constructs from a given integer.
  ezSimdFloat(ezInt32 i); // [tested]

  /// \brief Constructs from a given integer.
  ezSimdFloat(ezUInt32 i); // [tested]

  /// \brief Constructs from given angle.
  ezSimdFloat(ezAngle a); // [tested]

  /// \brief Constructs from the internal implementation type.
  ezSimdFloat(ezInternal::QuadFloat v); // [tested]

  /// \brief Returns the stored number as a standard float.
  operator float() const; // [tested]

  static ezSimdFloat Zero(); // [tested]

public:
  ezSimdFloat operator+(const ezSimdFloat& f) const; // [tested]
  ezSimdFloat operator-(const ezSimdFloat& f) const; // [tested]
  ezSimdFloat operator*(const ezSimdFloat& f) const; // [tested]
  ezSimdFloat operator/(const ezSimdFloat& f) const; // [tested]

  ezSimdFloat& operator+=(const ezSimdFloat& f); // [tested]
  ezSimdFloat& operator-=(const ezSimdFloat& f); // [tested]
  ezSimdFloat& operator*=(const ezSimdFloat& f); // [tested]
  ezSimdFloat& operator/=(const ezSimdFloat& f); // [tested]

  bool IsEqual(const ezSimdFloat& rhs, const ezSimdFloat& fEpsilon) const;

  bool operator==(const ezSimdFloat& f) const; // [tested]
  bool operator!=(const ezSimdFloat& f) const; // [tested]
  bool operator>(const ezSimdFloat& f) const;  // [tested]
  bool operator>=(const ezSimdFloat& f) const; // [tested]
  bool operator<(const ezSimdFloat& f) const;  // [tested]
  bool operator<=(const ezSimdFloat& f) const; // [tested]

  bool operator==(float f) const; // [tested]
  bool operator!=(float f) const; // [tested]
  bool operator>(float f) const;  // [tested]
  bool operator>=(float f) const; // [tested]
  bool operator<(float f) const;  // [tested]
  bool operator<=(float f) const; // [tested]

  template <ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetReciprocal() const; // [tested]

  template <ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetSqrt() const; // [tested]

  template <ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetInvSqrt() const; // [tested]

  ezSimdFloat Max(const ezSimdFloat& f) const; // [tested]
  ezSimdFloat Min(const ezSimdFloat& f) const; // [tested]
  ezSimdFloat Abs() const;                     // [tested]

public:
  ezInternal::QuadFloat m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEFloat_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUFloat_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
