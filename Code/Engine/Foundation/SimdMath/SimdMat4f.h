#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// \brief A 4x4 matrix class
class EZ_FOUNDATION_DLL ezSimdMat4f
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdMat4f();

  ezSimdMat4f(const float* const pData, ezMatrixLayout::Enum layout); // [tested]

  ezSimdMat4f(const ezSimdVec4f& vCol0, const ezSimdVec4f& vCol1, const ezSimdVec4f& vCol2, const ezSimdVec4f& vCol3); // [tested]

  /// \brief Sets each element manually: Naming is "column-n row-m"
  ezSimdMat4f(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3, float f2r3, float f3r3,
    float f4r3, float f1r4, float f2r4, float f3r4, float f4r4); // [tested]

  void SetFromArray(const float* const pData, ezMatrixLayout::Enum layout); // [tested]

  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

  /// \brief Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Matrix.
  static ezSimdMat4f IdentityMatrix(); // [tested]

public:
  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  ezSimdMat4f GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be inverted.
  ezResult Invert(const ezSimdFloat& fEpsilon = ezMath::SmallEpsilon<float>()); // [tested]

  /// \brief Returns the inverse of this matrix.
  ezSimdMat4f GetInverse(const ezSimdFloat& fEpsilon = ezMath::SmallEpsilon<float>()) const; // [tested]

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const ezSimdMat4f& rhs, const ezSimdFloat& fEpsilon) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(const ezSimdFloat& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

public:
  void SetRows(const ezSimdVec4f& vRow0, const ezSimdVec4f& vRow1, const ezSimdVec4f& vRow2, const ezSimdVec4f& vRow3); // [tested]
  void GetRows(ezSimdVec4f& ref_vRow0, ezSimdVec4f& ref_vRow1, ezSimdVec4f& ref_vRow2, ezSimdVec4f& ref_vRow3) const;   // [tested]

public:
  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  ezSimdVec4f TransformPosition(const ezSimdVec4f& v) const; // [tested]

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  ezSimdVec4f TransformDirection(const ezSimdVec4f& v) const; // [tested]

  ezSimdMat4f operator*(const ezSimdMat4f& rhs) const; // [tested]
  void operator*=(const ezSimdMat4f& rhs);

  bool operator==(const ezSimdMat4f& rhs) const; // [tested]
  bool operator!=(const ezSimdMat4f& rhs) const; // [tested]

public:
  ezSimdVec4f m_col0;
  ezSimdVec4f m_col1;
  ezSimdVec4f m_col2;
  ezSimdVec4f m_col3;
};

#include <Foundation/SimdMath/Implementation/SimdMat4f_inl.h>

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4f_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
