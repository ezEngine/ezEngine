#pragma once

#include <Foundation/SimdMath/SimdDouble.h>
#include <Foundation/SimdMath/SimdVec4b_Wide.h>

/// \brief A 4-component SIMD vector class using doubles
class EZ_FOUNDATION_DLL ezSimdVec4d
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4d();

  explicit ezSimdVec4d(float fXyzw);
  explicit ezSimdVec4d(double fXyzw);

  explicit ezSimdVec4d(const ezSimdDouble& fXyzw);

  ezSimdVec4d(int x, int y, int z, int w = 1);
  ezSimdVec4d(float x, float y, float z, float w = 1.0f);
  ezSimdVec4d(double x, double y, double z, double w = 1.0);

  ezSimdVec4d(ezInternal::QuadDouble v);

  /// \brief Creates an ezSimdVec4d that is initialized to zero.
  [[nodiscard]] static ezSimdVec4d MakeZero();

  /// \brief Creates an ezSimdVec4d that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static ezSimdVec4d MakeNaN();

  void Set(float fXyzw);
  void Set(double fXyzw);

  void Set(int x, int y, int z, int w);
  void Set(float x, float y, float z, float w);
  void Set(double x, double y, double z, double w);

  void SetX(const ezSimdDouble& f);
  void SetY(const ezSimdDouble& f);
  void SetZ(const ezSimdDouble& f);
  void SetW(const ezSimdDouble& f);

  void SetZero();

  /// \brief Loads N floats from pFloats, converts them to double, and stores in the vector.
  /// N must be between 1 and 4. Unused components are set to zero.
  template <int N>
  void Load(const float* pFloats);

  /// \brief Loads N doubles from pDoubles into the vector.
  /// N must be between 1 and 4. Unused components are set to zero.
  template <int N>
  void Load(const double* pDoubles);

  /// \brief Converts the first N components to float and stores them to pFloats.
  /// N must be between 1 and 4.
  template <int N>
  void Store(float* pFloats) const;

  /// \brief Stores the first N components to pDoubles.
  /// N must be between 1 and 4.
  template <int N>
  void Store(double* pDoubles) const;

public:
  ezSimdVec4d GetReciprocal() const;

  ezSimdVec4d GetSqrt() const;

  ezSimdVec4d GetInvSqrt() const;

  template <int N>
  ezSimdDouble GetLength() const;

  template <int N>
  ezSimdDouble GetInvLength() const;

  template <int N>
  ezSimdDouble GetLengthSquared() const;

  template <int N>
  ezSimdDouble GetLengthAndNormalize();

  template <int N>
  ezSimdVec4d GetNormalized() const;

  template <int N>
  void Normalize();

  /// \brief Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to zero.
  template <int N>
  void NormalizeIfNotZero(const ezSimdDouble& fEpsilon = ezMath::SmallEpsilon<double>());

  /// \brief Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to vFallback.
  template <int N>
  void NormalizeIfNotZero(const ezSimdVec4d& vFallback, const ezSimdDouble& fEpsilon = ezMath::SmallEpsilon<double>());

  template <int N>
  bool IsZero() const;

  template <int N>
  bool IsZero(const ezSimdDouble& fEpsilon) const;

  template <int N>
  bool IsNormalized(const ezSimdDouble& fEpsilon = ezMath::HugeEpsilon<double>()) const;

  template <int N>
  bool IsNaN() const;

  template <int N>
  bool IsValid() const;

public:
  template <int N>
  ezSimdDouble GetComponent() const;

  ezSimdDouble GetComponent(int i) const;

  ezSimdDouble x() const;
  ezSimdDouble y() const;
  ezSimdDouble z() const;
  ezSimdDouble w() const;

  template <ezSwizzle::Enum s>
  ezSimdVec4d Get() const;

  ///\brief x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <ezSwizzle::Enum s>
  [[nodiscard]] ezSimdVec4d GetCombined(const ezSimdVec4d& other) const;

public:
  [[nodiscard]] ezSimdVec4d operator-() const;
  [[nodiscard]] ezSimdVec4d operator+(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4d operator-(const ezSimdVec4d& v) const;

  [[nodiscard]] ezSimdVec4d operator*(const ezSimdDouble& f) const;
  [[nodiscard]] ezSimdVec4d operator/(const ezSimdDouble& f) const;

  [[nodiscard]] ezSimdVec4d CompMul(const ezSimdVec4d& v) const;

  [[nodiscard]] ezSimdVec4d CompDiv(const ezSimdVec4d& v) const;

  [[nodiscard]] ezSimdVec4d CompMin(const ezSimdVec4d& rhs) const;
  [[nodiscard]] ezSimdVec4d CompMax(const ezSimdVec4d& rhs) const;

  [[nodiscard]] ezSimdVec4d Abs() const;
  [[nodiscard]] ezSimdVec4d Round() const;
  [[nodiscard]] ezSimdVec4d Floor() const;
  [[nodiscard]] ezSimdVec4d Ceil() const;
  [[nodiscard]] ezSimdVec4d Trunc() const;
  [[nodiscard]] ezSimdVec4d Fraction() const;

  [[nodiscard]] ezSimdVec4d FlipSign(const ezSimdVec4bWide& vCmp) const;

  [[nodiscard]] static ezSimdVec4d Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4d& vTrue, const ezSimdVec4d& vFalse);

  [[nodiscard]] static ezSimdVec4d Lerp(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& t);

  ezSimdVec4d& operator+=(const ezSimdVec4d& v);
  ezSimdVec4d& operator-=(const ezSimdVec4d& v);

  ezSimdVec4d& operator*=(const ezSimdDouble& f);
  ezSimdVec4d& operator/=(const ezSimdDouble& f);

  ezSimdVec4bWide IsEqual(const ezSimdVec4d& rhs, const ezSimdDouble& fEpsilon) const;

  [[nodiscard]] ezSimdVec4bWide operator==(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4bWide operator!=(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4bWide operator<=(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4bWide operator<(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4bWide operator>=(const ezSimdVec4d& v) const;
  [[nodiscard]] ezSimdVec4bWide operator>(const ezSimdVec4d& v) const;

  /// \brief Returns the sum of the first N components.
  template <int N>
  [[nodiscard]] ezSimdDouble HorizontalSum() const;

  /// \brief Returns the minimum of the first N components.
  template <int N>
  [[nodiscard]] ezSimdDouble HorizontalMin() const;

  /// \brief Returns the maximum of the first N components.
  template <int N>
  [[nodiscard]] ezSimdDouble HorizontalMax() const;

  /// \brief Returns the dot product of the first N components of this vector and v.
  template <int N>
  [[nodiscard]] ezSimdDouble Dot(const ezSimdVec4d& v) const;

  ///\brief 3D cross product, w is ignored.
  [[nodiscard]] ezSimdVec4d CrossRH(const ezSimdVec4d& v) const;

  ///\brief Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  [[nodiscard]] ezSimdVec4d GetOrthogonalVector() const;

  /// \brief Returns a * b + c
  [[nodiscard]] static ezSimdVec4d MulAdd(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c);
  [[nodiscard]] static ezSimdVec4d MulAdd(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c);

  /// \brief Returns a * b - c
  [[nodiscard]] static ezSimdVec4d MulSub(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c);
  [[nodiscard]] static ezSimdVec4d MulSub(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c);

  /// \brief Returns a vector with the magnitude from vMagnitude and the sign from vSign.
  [[nodiscard]] static ezSimdVec4d CopySign(const ezSimdVec4d& vMagnitude, const ezSimdVec4d& vSign);

public:
  ezInternal::QuadDouble m_v;
};

const ezSimdVec4d operator*(const ezSimdDouble& f, const ezSimdVec4d& v);

#include <Foundation/SimdMath/Implementation/SimdVec4d_inl.h>

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4d_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4d_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif