#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// A 4x4 matrix class
class EZ_FOUNDATION_DLL ezSimdMat4f
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdMat4f();

  /// Returns a zero matrix.
  [[nodiscard]] static ezSimdMat4f MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static ezSimdMat4f MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static ezSimdMat4f MakeFromRowMajorArray(const float* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static ezSimdMat4f MakeFromColumnMajorArray(const float* const pData);

  /// Creates a matrix from 4 column vectors.
  [[nodiscard]] static ezSimdMat4f MakeFromColumns(const ezSimdVec4f& vCol0, const ezSimdVec4f& vCol1, const ezSimdVec4f& vCol2, const ezSimdVec4f& vCol3);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static ezSimdMat4f MakeFromValues(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3, float f2r3, float f3r3, float f4r3, float f1r4, float f2r4, float f3r4, float f4r4);

  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  ezSimdMat4f GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  ezResult Invert(const ezSimdFloat& fEpsilon = ezMath::SmallEpsilon<float>()); // [tested]

  /// Returns the inverse of this matrix.
  ezSimdMat4f GetInverse(const ezSimdFloat& fEpsilon = ezMath::SmallEpsilon<float>()) const; // [tested]

public:
  /// Equality Check with epsilon
  bool IsEqual(const ezSimdMat4f& rhs, const ezSimdFloat& fEpsilon) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(const ezSimdFloat& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const;                                                                                                   // [tested]

public:
  void SetRows(const ezSimdVec4f& vRow0, const ezSimdVec4f& vRow1, const ezSimdVec4f& vRow2, const ezSimdVec4f& vRow3); // [tested]
  void GetRows(ezSimdVec4f& ref_vRow0, ezSimdVec4f& ref_vRow1, ezSimdVec4f& ref_vRow2, ezSimdVec4f& ref_vRow3) const;   // [tested]

public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] ezSimdVec4f TransformPosition(const ezSimdVec4f& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] ezSimdVec4f TransformDirection(const ezSimdVec4f& v) const; // [tested]

  [[nodiscard]] ezSimdMat4f operator*(const ezSimdMat4f& rhs) const;        // [tested]
  void operator*=(const ezSimdMat4f& rhs);

  [[nodiscard]] bool operator==(const ezSimdMat4f& rhs) const;              // [tested]
  [[nodiscard]] bool operator!=(const ezSimdMat4f& rhs) const;              // [tested]

public:
  ezSimdVec4f m_col0;
  ezSimdVec4f m_col1;
  ezSimdVec4f m_col2;
  ezSimdVec4f m_col3;
};

/// Multiply two affine matrices, where the 4th row of each is 0,0,0,1.
[[nodiscard]] ezSimdMat4f MultiplyAffine(const ezSimdMat4f& lhs, const ezSimdMat4f& rhs);

#include <Foundation/SimdMath/Implementation/SimdMat4f_inl.h>

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4f_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4f_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
