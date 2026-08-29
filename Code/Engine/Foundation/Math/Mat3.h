#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

/// A 3x3 component matrix class.
///
/// Matrix layout (column-major):
/// ```
/// | m00 m10 m20 |   Column 0: (m00, m01, m02)
/// | m01 m11 m21 |   Column 1: (m10, m11, m12)
/// | m02 m12 m22 |   Column 2: (m20, m21, m22)
/// ```
template <typename Type>
class ezMat3Template
{
public:
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2),
  // then column 1, then column 2

  /// The matrix as a 9-element Type array (column-major)
  Type m_fElementsCM[9];

  EZ_ALWAYS_INLINE Type& Element(ezInt32 iColumn, ezInt32 iRow) { return m_fElementsCM[iColumn * 3 + iRow]; }
  EZ_ALWAYS_INLINE Type Element(ezInt32 iColumn, ezInt32 iRow) const { return m_fElementsCM[iColumn * 3 + iRow]; }

  // *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  ezMat3Template(); // [tested]

  /// Returns a zero matrix.
  [[nodiscard]] static ezMat3Template<Type> MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static ezMat3Template<Type> MakeIdentity();

  /// Creates a matrix from 9 values that are in row-major layout.
  [[nodiscard]] static ezMat3Template<Type> MakeFromRowMajorArray(const Type* const pData);

  /// Creates a matrix from 9 values that are in column-major layout.
  [[nodiscard]] static ezMat3Template<Type> MakeFromColumnMajorArray(const Type* const pData);

  /// Creates a matrix from 9 values. Naming is "column-n row-m"
  [[nodiscard]] static ezMat3Template<Type> MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3);

  /// Creates a matrix with all zero values, except along the diagonal, which is set to x,y,z
  [[nodiscard]] static ezMat3Template<Type> MakeScaling(const ezVec3Template<Type>& vScale);

  /// Creates a matrix that is a rotation matrix around the X-axis.
  [[nodiscard]] static ezMat3Template<Type> MakeRotationX(ezAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Y-axis.
  [[nodiscard]] static ezMat3Template<Type> MakeRotationY(ezAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Z-axis.
  [[nodiscard]] static ezMat3Template<Type> MakeRotationZ(ezAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the given axis.
  [[nodiscard]] static ezMat3Template<Type> MakeAxisRotation(const ezVec3Template<Type>& vAxis, ezAngleTemplate<Type> angle);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Copies the 9 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major
  /// format.
  void GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

  // *** Special matrix constructors ***
public:
  /// Sets all elements to zero.
  void SetZero(); // [tested]

  /// Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  // *** Common Matrix Operations ***
public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  const ezMat3Template<Type> GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be Inverted.
  ezResult Invert(Type fEpsilon = ezMath::SmallEpsilon<Type>()); // [tested]

  /// Returns the inverse of this matrix.
  const ezMat3Template<Type> GetInverse(Type fEpsilon = ezMath::SmallEpsilon<Type>()) const; // [tested]

  // *** Checks ***
public:
  /// Checks whether all elements are zero.
  bool IsZero(Type fEpsilon = ezMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(Type fEpsilon = ezMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  // *** Special Accessors ***
public:
  /// Returns all 3 components of the i-th row.
  ezVec3Template<Type> GetRow(ezUInt32 uiRow) const; // [tested]

  /// Sets all 3 components of the i-th row.
  void SetRow(ezUInt32 uiRow, const ezVec3Template<Type>& vRow); // [tested]

  /// Returns all 3 components of the i-th column.
  ezVec3Template<Type> GetColumn(ezUInt32 uiColumn) const; // [tested]

  /// Sets all 3 components of the i-th column.
  void SetColumn(ezUInt32 uiColumn, const ezVec3Template<Type>& vColumn); // [tested]

  /// Returns all 3 components on the diagonal of the matrix.
  ezVec3Template<Type> GetDiagonal() const; // [tested]

  /// Sets all 3 components on the diagonal of the matrix.
  void SetDiagonal(const ezVec3Template<Type>& vDiag); // [tested]

  /// Returns the 3 scaling factors that are encoded in the matrix.
  const ezVec3Template<Type> GetScalingFactors() const; // [tested]

  /// Tries to set the three scaling factors in the matrix. Returns EZ_FAILURE if the matrix columns cannot be normalized and thus no rescaling
  /// is possible.
  ezResult SetScalingFactors(const ezVec3Template<Type>& vXYZ, Type fEpsilon = ezMath::DefaultEpsilon<Type>()); // [tested]

  /// Computes the determinant of the matrix.
  Type GetDeterminant() const;

  // *** Operators ***
public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  const ezVec3Template<Type> TransformDirection(const ezVec3Template<Type>& v) const; // [tested]

  /// Component-wise multiplication (commutative)
  void operator*=(Type f);

  /// Component-wise division.
  void operator/=(Type f); // [tested]

  /// Equality Check.
  bool IsIdentical(const ezMat3Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon.
  bool IsEqual(const ezMat3Template<Type>& rhs, Type fEpsilon) const; // [tested]
};

// *** free functions ***

/// Matrix-Matrix multiplication
template <typename Type>
const ezMat3Template<Type> operator*(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const ezVec3Template<Type> operator*(const ezMat3Template<Type>& m, const ezVec3Template<Type>& v); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const ezMat3Template<Type> operator*(const ezMat3Template<Type>& m1, Type f); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const ezMat3Template<Type> operator*(Type f, const ezMat3Template<Type>& m1); // [tested]

/// Component-wise division
template <typename Type>
const ezMat3Template<Type> operator/(const ezMat3Template<Type>& m1, Type f); // [tested]

/// Adding two matrices (component-wise)
template <typename Type>
const ezMat3Template<Type> operator+(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// Subtracting two matrices (component-wise)
template <typename Type>
const ezMat3Template<Type> operator-(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// Comparison Operator ==
template <typename Type>
bool operator==(const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs); // [tested]

/// Comparison Operator !=
template <typename Type>
bool operator!=(const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Mat3_inl.h>
