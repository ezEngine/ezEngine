#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class EZ_FOUNDATION_DLL ezSimdDouble
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor, leaves the data uninitialized.
  ezSimdDouble();

  /// \brief Constructs from a given float.
  ezSimdDouble(float f);

  /// \brief Constructs from a given double.
  ezSimdDouble(double f);

  /// \brief Constructs from a given integer.
  ezSimdDouble(ezInt32 i);

  /// \brief Constructs from a given integer.
  ezSimdDouble(ezUInt32 i);

  /// \brief Constructs from given angle.
  ezSimdDouble(ezAngle a);

  /// \brief Constructs from smaller SIMD
  ezSimdDouble(ezInternal::QuadFloat v);

  /// \brief Constructs from the internal implementation type.
  ezSimdDouble(ezInternal::QuadDouble v);

  // /// \brief Returns the stored number as a standard float.
  // operator float() const;

  /// \brief Returns the stored number as a standard double.
  operator double() const;

  /// \brief Creates an ezSimdDouble that is initialized to zero.
  [[nodiscard]] static ezSimdDouble MakeZero();

  /// \brief Creates an ezSimdDouble that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static ezSimdDouble MakeNaN();

public:
  ezSimdDouble operator+(const ezSimdDouble& f) const;
  ezSimdDouble operator-(const ezSimdDouble& f) const;
  ezSimdDouble operator*(const ezSimdDouble& f) const;
  ezSimdDouble operator/(const ezSimdDouble& f) const;

  ezSimdDouble& operator+=(const ezSimdDouble& f);
  ezSimdDouble& operator-=(const ezSimdDouble& f);
  ezSimdDouble& operator*=(const ezSimdDouble& f);
  ezSimdDouble& operator/=(const ezSimdDouble& f);

  bool IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& fEpsilon) const;

  bool operator==(const ezSimdDouble& f) const;
  bool operator!=(const ezSimdDouble& f) const;
  bool operator>(const ezSimdDouble& f) const;
  bool operator>=(const ezSimdDouble& f) const;
  bool operator<(const ezSimdDouble& f) const;
  bool operator<=(const ezSimdDouble& f) const;

  bool operator==(double f) const;
  bool operator!=(double f) const;
  bool operator>(double f) const;
  bool operator>=(double f) const;
  bool operator<(double f) const;
  bool operator<=(double f) const;

  bool operator==(float f) const;
  bool operator!=(float f) const;
  bool operator>(float f) const;
  bool operator>=(float f) const;
  bool operator<(float f) const;
  bool operator<=(float f) const;

  ezSimdDouble GetReciprocal() const;

  ezSimdDouble GetSqrt() const;

  ezSimdDouble GetInvSqrt() const;

  [[nodiscard]] ezSimdDouble Max(const ezSimdDouble& d) const;
  [[nodiscard]] ezSimdDouble Min(const ezSimdDouble& d) const;
  [[nodiscard]] ezSimdDouble Abs() const;

public:
  ezInternal::QuadDouble m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  if EZ_SSE_LEVEL >= EZ_SSE_AVX
#    include <Foundation/SimdMath/Implementation/SSE/SSEDouble_AVX_inl.h>
#  else
#    include <Foundation/SimdMath/Implementation/SSE/SSEDouble_inl.h>
#  endif
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUDouble_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONDouble_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
