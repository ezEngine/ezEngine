#pragma once

#include <Foundation/SimdMath/SimdVec4d.h>

/// \brief A 4x4 matrix class
class EZ_FOUNDATION_DLL ezSimdMat4d
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdMat4d();

  /// \brief Returns a zero matrix.
  [[nodiscard]] static ezSimdMat4d MakeZero();

  /// \brief Returns an identity matrix.
  [[nodiscard]] static ezSimdMat4d MakeIdentity();

  /// \brief Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static ezSimdMat4d MakeFromRowMajorArray(const double* const pData);

  /// \brief Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static ezSimdMat4d MakeFromColumnMajorArray(const double* const pData);

  /// \brief Creates a matrix from 4 column vectors.
  [[nodiscard]] static ezSimdMat4d MakeFromColumns(const ezSimdVec4d& vCol0, const ezSimdVec4d& vCol1, const ezSimdVec4d& vCol2, const ezSimdVec4d& vCol3);

  /// \brief Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static ezSimdMat4d MakeFromValues(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3, double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4);

  void GetAsArray(double* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

public:
  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  ezSimdMat4d GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be inverted.
  ezResult Invert(const ezSimdDouble& fEpsilon = ezMath::SmallEpsilon<double>()); // [tested]

  /// \brief Returns the inverse of this matrix.
  ezSimdMat4d GetInverse(const ezSimdDouble& fEpsilon = ezMath::SmallEpsilon<double>()) const; // [tested]

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const ezSimdMat4d& rhs, const ezSimdDouble& fEpsilon) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(const ezSimdDouble& fEpsilon = ezMath::DefaultEpsilon<double>()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const;                                                                                                   // [tested]

public:
  void SetRows(const ezSimdVec4d& vRow0, const ezSimdVec4d& vRow1, const ezSimdVec4d& vRow2, const ezSimdVec4d& vRow3); // [tested]
  void GetRows(ezSimdVec4d& ref_vRow0, ezSimdVec4d& ref_vRow1, ezSimdVec4d& ref_vRow2, ezSimdVec4d& ref_vRow3) const;   // [tested]

public:
  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] ezSimdVec4d TransformPosition(const ezSimdVec4d& v) const; // [tested]

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] ezSimdVec4d TransformDirection(const ezSimdVec4d& v) const; // [tested]

  [[nodiscard]] ezSimdMat4d operator*(const ezSimdMat4d& rhs) const;        // [tested]
  void operator*=(const ezSimdMat4d& rhs);

  [[nodiscard]] bool operator==(const ezSimdMat4d& rhs) const;              // [tested]
  [[nodiscard]] bool operator!=(const ezSimdMat4d& rhs) const;              // [tested]

public:
  ezSimdVec4d m_col0;
  ezSimdVec4d m_col1;
  ezSimdVec4d m_col2;
  ezSimdVec4d m_col3;
};

/// \brief Multiply two affine matrices, where the 4th row of each is 0,0,0,1.
[[nodiscard]] ezSimdMat4d MultiplyAffine(const ezSimdMat4d& lhs, const ezSimdMat4d& rhs);

#include <Foundation/SimdMath/Implementation/SimdMat4d_inl.h>

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4d_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4d_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
