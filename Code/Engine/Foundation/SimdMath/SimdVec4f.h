#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// \brief A 4-component SIMD vector class
class EZ_FOUNDATION_DLL ezSimdVec4f
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

  void SetX(const ezSimdFloat& f); // [tested]
  void SetY(const ezSimdFloat& f); // [tested]
  void SetZ(const ezSimdFloat& f); // [tested]
  void SetW(const ezSimdFloat& f); // [tested]

  void SetZero(); // [tested]

  template<int N>
  void Load(const float* pFloats); // [tested]

  template<int N>
  void Store(float* pFloats) const; // [tested]

public:
  template<ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f GetReciprocal() const; // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetLength() const; // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetInvLength() const; // [tested]

  template<int N>
  ezSimdFloat GetLengthSquared() const; // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdFloat GetLengthAndNormalize(); // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f GetNormalized() const; // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  void Normalize(); // [tested]

  template<int N, ezMathAcc::Enum acc = ezMathAcc::FULL>
  void NormalizeIfNotZero(const ezSimdFloat& fEpsilon = ezMath::BasicType<float>::SmallEpsilon()); // [tested]

  template<int N>
  bool IsZero() const; // [tested]

  template<int N>
  bool IsZero(const ezSimdFloat& fEpsilon) const; // [tested]

  template<int N>
  bool IsNormalized(const ezSimdFloat& fEpsilon = ezMath::BasicType<float>::HugeEpsilon()) const; // [tested]

  template<int N>
  bool IsNaN() const; // [tested]

  template<int N>
  bool IsValid() const; // [tested]

public:
  template<int N>
  ezSimdFloat GetComponent() const; // [tested]

  ezSimdFloat GetComponent(int i) const; // [tested]

  ezSimdFloat x() const; // [tested]
  ezSimdFloat y() const; // [tested]
  ezSimdFloat z() const; // [tested]
  ezSimdFloat w() const; // [tested]

  template <ezSwizzle::Enum s>
  ezSimdVec4f Get() const; // [tested]

public:
  ezSimdVec4f operator-() const; // [tested]
  ezSimdVec4f operator+(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4f operator-(const ezSimdVec4f& v) const; // [tested]

  ezSimdVec4f operator*(const ezSimdFloat& f) const; // [tested]
  ezSimdVec4f operator/(const ezSimdFloat& f) const; // [tested]

  ezSimdVec4f CompMul(const ezSimdVec4f& v) const; // [tested]

  template<ezMathAcc::Enum acc = ezMathAcc::FULL>
  ezSimdVec4f CompDiv(const ezSimdVec4f& v) const; // [tested]

  ezSimdVec4f CompMin(const ezSimdVec4f& rhs) const; // [tested]
  ezSimdVec4f CompMax(const ezSimdVec4f& rhs) const; // [tested]
  ezSimdVec4f Abs() const; // [tested]
  ezSimdVec4f Floor() const; // [tested]
  ezSimdVec4f Ceil() const; // [tested]

  ezSimdVec4f FlipSign(const ezSimdVec4b& cmp) const; // [tested]

  static ezSimdVec4f Select(const ezSimdVec4f& ifFalse, const ezSimdVec4f& ifTrue, const ezSimdVec4b& cmp); // [tested]

  ezSimdVec4f& operator+=(const ezSimdVec4f& v); // [tested]
  ezSimdVec4f& operator-=(const ezSimdVec4f& v); // [tested]

  ezSimdVec4f& operator*=(const ezSimdFloat& f); // [tested]
  ezSimdVec4f& operator/=(const ezSimdFloat& f); // [tested]

  ezSimdVec4b IsEqual(const ezSimdVec4f& rhs, const ezSimdFloat& fEpsilon) const; // [tested]

  ezSimdVec4b operator==(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4b operator!=(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4b operator<=(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4b operator<(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4b operator>=(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4b operator>(const ezSimdVec4f& v) const; // [tested]

  template<int N>
  ezSimdFloat HorizontalSum() const; // [tested]

  template<int N>
  ezSimdFloat HorizontalMin() const; // [tested]

  template<int N>
  ezSimdFloat HorizontalMax() const; // [tested]

  template<int N>
  ezSimdFloat Dot(const ezSimdVec4f& v) const; // [tested]

  ///\brief 3D cross product, w is ignored.
  ezSimdVec4f Cross(const ezSimdVec4f& v) const; // [tested]

  ///\brief Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  ezSimdVec4f GetOrthogonalVector() const; // [tested]

  static ezSimdVec4f ZeroVector(); // [tested]

  static ezSimdVec4f MulAdd(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c); // [tested]
  static ezSimdVec4f MulAdd(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c); // [tested]

  static ezSimdVec4f MulSub(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c); // [tested]
  static ezSimdVec4f MulSub(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c); // [tested]

  static ezSimdVec4f CopySign(const ezSimdVec4f& magnitude, const ezSimdVec4f& sign); // [tested]

public:
  ezInternal::QuadFloat m_v;
};

#include <Foundation/SimdMath/Implementation/SimdVec4f_inl.h>

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  #include <Foundation/SimdMath/Implementation/SSE/SSEVec4f_inl.h>
#else
  #include <Foundation/SimdMath/Implementation/FPU/FPUVec4f_inl.h>
#endif

