#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// \brief A 4-component SIMD vector class
class ezSimdVec4f
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4f(); // [tested]

  explicit ezSimdVec4f(float xyzw); // [tested]

  explicit ezSimdVec4f(const ezSimdFloat& xyzw); // [tested]

  ezSimdVec4f(float x, float y, float z, float w = 1.0f); // [tested]

  ezSimdVec4f(ezInternal::QuadFloat v); // [tested]

  void Set(float xyzw); // [tested]

  void Set(float x, float y, float z, float w); // [tested]

  void SetZero(); // [tested]

  template<int N>
  void Load(const float* pFloats); // [tested]

  template<int N>
  void Store(float* pFloats) const; // [tested]

public:
  template<ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f GetReciprocal() const;

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetLength() const;

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetInvLength() const;

  template<int N>
  ezSimdFloat GetLengthSquared() const;

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetLengthAndNormalize();

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f GetNormalized() const;

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  void Normalize();

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezResult NormalizeIfNotZero(const ezSimdVec4f& vFallback = ezSimdVec4f(1, 0, 0, 0), const ezSimdFloat& fEpsilon = ezMath::BasicType<float>::SmallEpsilon());

  template<int N>
  bool IsZero() const;

  template<int N>
  bool IsZero(const ezSimdFloat& fEpsilon) const;

  template<int N>
  bool IsNormalized(const ezSimdFloat& fEpsilon = ezMath::BasicType<float>::HugeEpsilon()) const;

  template<int N>
  bool IsNaN() const;

  template<int N>
  bool IsValid() const;

public:
  template<int N>
  ezSimdFloat GetComponent() const;

  ezSimdFloat GetComponent(int i) const;

  ezSimdFloat x() const;
  ezSimdFloat y() const;
  ezSimdFloat z() const;
  ezSimdFloat w() const;

  template <ezSwizzle::Enum s>
  ezSimdVec4f Get() const;

public:
  ezSimdVec4f operator+(const ezSimdVec4f& v) const;
  ezSimdVec4f operator-(const ezSimdVec4f& v) const;

  ezSimdVec4f operator*(const ezSimdFloat& f) const;
  ezSimdVec4f operator/(const ezSimdFloat& f) const;

  ezSimdVec4f CompMul(const ezSimdVec4f& v) const;

  template<ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f CompDiv(const ezSimdVec4f& v) const;

  ezSimdVec4f CompMin(const ezSimdVec4f& rhs) const;
  ezSimdVec4f CompMax(const ezSimdVec4f& rhs) const;
  ezSimdVec4f Abs() const;

  ezSimdVec4f FlipSign(const ezSimdVec4b& cmp) const;

  static ezSimdVec4f Select(const ezSimdVec4f& ifFalse, const ezSimdVec4f& ifTrue, const ezSimdVec4b& cmp);

  ezSimdVec4f& operator+=(const ezSimdVec4f& v);
  ezSimdVec4f& operator-=(const ezSimdVec4f& v);

  ezSimdVec4f& operator*=(const ezSimdFloat& f);
  ezSimdVec4f& operator/=(const ezSimdFloat& f);

  ezSimdVec4b IsEqual(const ezSimdVec4f& rhs, const ezSimdFloat& fEpsilon) const;

  ezSimdVec4b operator==(const ezSimdVec4f& v) const;
  ezSimdVec4b operator!=(const ezSimdVec4f& v) const;
  ezSimdVec4b operator<=(const ezSimdVec4f& v) const;
  ezSimdVec4b operator<(const ezSimdVec4f& v) const;
  ezSimdVec4b operator>=(const ezSimdVec4f& v) const;
  ezSimdVec4b operator>(const ezSimdVec4f& v) const;

  template<int N>
  ezSimdFloat HorizontalSum() const;

  template<int N>
  ezSimdFloat HorizontalMin() const;

  template<int N>
  ezSimdFloat HorizontalMax() const;

  template<int N>
  ezSimdFloat Dot(const ezSimdVec4f& v) const;

  ezSimdVec4f Cross(const ezSimdVec4f& v) const;

  /// Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  ezSimdVec4f GetOrthogonalVector() const;

  static ezSimdVec4f ZeroVector();

  static ezSimdVec4f MulAdd(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c);
  static ezSimdVec4f MulAdd(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c);

  static ezSimdVec4f MulSub(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c);
  static ezSimdVec4f MulSub(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c);

  static ezSimdVec4f CopySign(const ezSimdVec4f& magnitude, const ezSimdVec4f& sign);

public:
  ezInternal::QuadFloat m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  #include <Foundation/SimdMath/Implementation/SSE/SSEVec4f_inl.h>
#else
  #include <Foundation/SimdMath/Implementation/FPU/FPUVec4f_inl.h>
#endif
